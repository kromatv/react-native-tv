import { createReadStream } from 'node:fs';
import { stat } from 'node:fs/promises';
import { Transform } from 'node:stream';
import { TvCommandError } from './luna.ts';
import { SftpUnavailableError, type SshSession } from './ssh-session.ts';

const PUSH_TIMEOUT_MS = 300_000;
const SAFE_PATH = /^[A-Za-z0-9_./-]+$/;

function counting(onBytes: (sent: number) => void): Transform {
  let sent = 0;
  return new Transform({
    transform(chunk: Buffer, _encoding, done) {
      sent += chunk.length;
      onBytes(sent);
      done(null, chunk);
    },
  });
}

/**
 * Copies a local file to the TV: through SFTP, else, on a TV without
 * `sftp-server`, as the input of `cat`. `onProgress` sees the bytes sent.
 */
export async function pushFile(
  session: SshSession,
  localPath: string,
  remotePath: string,
  onProgress: (sent: number, total: number) => void = () => undefined,
): Promise<void> {
  if (!SAFE_PATH.test(remotePath)) throw new Error(`refusing to write ${remotePath} on the TV`);
  const total = (await stat(localPath)).size;
  const report = (sent: number) => onProgress(sent, total);
  const source = createReadStream(localPath);
  try {
    await session.sftpPut(source, remotePath, report);
    return;
  } catch (error) {
    source.destroy();
    if (!(error instanceof SftpUnavailableError)) throw error;
  }
  const counted = counting(report);
  createReadStream(localPath)
    .on('error', (error) => counted.destroy(error))
    .pipe(counted);
  const result = await session.exec(`/bin/cat > "${remotePath}"`, {
    stdin: counted,
    timeoutMs: PUSH_TIMEOUT_MS,
  });
  if (result.code !== 0) throw new TvCommandError(`The TV did not store ${remotePath}`, result);
}
