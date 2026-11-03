import { createHash } from 'node:crypto';
import { createReadStream, createWriteStream, existsSync } from 'node:fs';
import { mkdir, rename, rm } from 'node:fs/promises';
import { dirname } from 'node:path';
import { Readable, Transform } from 'node:stream';
import { pipeline } from 'node:stream/promises';
import { FixableError } from '../errors.ts';

const NETWORK_HINT = 'Check the network connection, then try again.';

async function sha256Of(path: string): Promise<string> {
  const hash = createHash('sha256');
  await pipeline(createReadStream(path), hash);
  return hash.digest('hex');
}

function hashing(hash: ReturnType<typeof createHash>): Transform {
  return new Transform({
    transform(chunk: Buffer, _encoding, done) {
      hash.update(chunk);
      done(null, chunk);
    },
  });
}

/**
 * Downloads `url` to `destination` unless a file with the expected digest is
 * already there. The archive only lands once its digest matched.
 */
export async function downloadVerified(
  url: string,
  sha256: string,
  destination: string,
): Promise<void> {
  if (existsSync(destination) && (await sha256Of(destination)) === sha256) return;
  const response = await fetch(url).catch((error: unknown) => {
    const reason = error instanceof Error ? error.message : String(error);
    throw new FixableError(`Downloading ${url} failed: ${reason}`, NETWORK_HINT);
  });
  if (!response.ok || response.body === null) {
    throw new FixableError(`Downloading ${url} failed: HTTP ${response.status}`, NETWORK_HINT);
  }
  await mkdir(dirname(destination), { recursive: true });
  const partial = `${destination}.part-${process.pid}`;
  const hash = createHash('sha256');
  try {
    const body = Readable.fromWeb(response.body);
    await pipeline(body, hashing(hash), createWriteStream(partial));
    const actual = hash.digest('hex');
    if (actual !== sha256) {
      throw new FixableError(
        `${url} does not match its pinned digest (got ${actual})`,
        'The download was corrupted or the archive changed upstream. Try again; report it if it persists.',
      );
    }
    await rename(partial, destination);
  } finally {
    await rm(partial, { force: true });
  }
}
