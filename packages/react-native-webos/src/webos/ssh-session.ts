import { createConnection, type Socket } from 'node:net';
import type { Readable } from 'node:stream';
import ssh2, {
  type Client,
  type ClientChannel,
  type KexAlgorithm,
  type SFTPWrapper,
} from 'ssh2';
import type { TvAddress } from './device-address.ts';

/** The login of a TV's Developer Mode SSH server. It takes a key and no password. */
export const DEVELOPER_USER = 'prisoner';

const CONNECT_TIMEOUT_MS = 5000;
const HANDSHAKE_TIMEOUT_MS = 30_000;
const EXEC_TIMEOUT_MS = 30_000;
const END_GRACE_MS = 2000;
const KEEPALIVE_IDLE_MS = 30_000;
const MAX_OUTPUT_BYTES = 1024 * 1024;
// LG's ares CLI offers every TV these two SHA-1 key exchanges, which ssh2 leaves out by default.
const LEGACY_KEY_EXCHANGES: KexAlgorithm[] = [
  'diffie-hellman-group14-sha1',
  'diffie-hellman-group1-sha1',
];

/** Why a TV's SSH server could not be used: nothing there, port closed, no handshake, or the key refused. */
export type SshFailure = 'unreachable' | 'closed' | 'silent' | 'denied';

/** A TV whose Developer Mode SSH server did not let `rn-webos` in. */
export class SshConnectError extends Error {
  readonly reason: SshFailure;

  constructor(reason: SshFailure, message: string) {
    super(message);
    this.name = 'SshConnectError';
    this.reason = reason;
  }
}

/** The TV has no SFTP subsystem for this login, so a file goes through `cat` instead. */
export class SftpUnavailableError extends Error {}

/** A TV's SSH server and the key it gave out. */
export interface SshTarget extends TvAddress {
  readonly privateKey: string;
}

/** How {@link SshSession.exec} runs a command. */
export interface ExecOptions {
  readonly timeoutMs?: number;
  /** What the command reads; without it its input stays open and silent until it ends. */
  readonly stdin?: Readable;
  /** Sees each complete line the command prints; returning true ends the command. */
  readonly onLine?: (line: string) => boolean | void;
}

/** What a command printed, and its exit status: null when it was ended or the TV reported none. */
export interface ExecResult {
  readonly code: number | null;
  readonly stdout: string;
  readonly stderr: string;
}

/** An SSH connection to a TV, logged in as `prisoner`. */
export interface SshSession {
  exec(command: string, options?: ExecOptions): Promise<ExecResult>;
  /** Writes a local file through SFTP; rejects with {@link SftpUnavailableError} when the TV has none. */
  sftpPut(
    source: Readable,
    remotePath: string,
    onProgress?: (bytes: number) => void,
  ): Promise<void>;
  close(): void;
}

const SOCKET_FAILURES: Readonly<Record<string, SshFailure>> = {
  ECONNREFUSED: 'closed',
  ECONNRESET: 'closed',
};

function openSocket(address: TvAddress): Promise<Socket> {
  return new Promise((resolve, reject) => {
    // A TV that drops off Wi-Fi sends no FIN, so without keepalive a waiting install hangs.
    const socket = createConnection({
      host: address.host,
      port: address.port,
      keepAlive: true,
      keepAliveInitialDelay: KEEPALIVE_IDLE_MS,
    });
    const fail = (reason: SshFailure, message: string) => {
      socket.destroy();
      reject(new SshConnectError(reason, message));
    };
    socket.setTimeout(CONNECT_TIMEOUT_MS);
    socket.once('timeout', () => fail('unreachable', 'the connection timed out'));
    socket.once('error', (error: NodeJS.ErrnoException) =>
      fail(SOCKET_FAILURES[error.code ?? ''] ?? 'unreachable', error.message),
    );
    socket.once('connect', () => {
      socket.setTimeout(0);
      socket.removeAllListeners('timeout');
      socket.removeAllListeners('error');
      resolve(socket);
    });
  });
}

function handshakeFailure(error: Error & { level?: string }): SshConnectError {
  if (error.level === 'client-authentication') return new SshConnectError('denied', error.message);
  return new SshConnectError('silent', error.message);
}

function lineSplitter(onLine: (line: string) => void): (chunk: string) => void {
  let pending = '';
  return (chunk) => {
    const lines = (pending + chunk).split(/\r?\n/);
    pending = lines.pop() ?? '';
    for (const line of lines) onLine(line);
  };
}

function runOnChannel(channel: ClientChannel, options: ExecOptions): Promise<ExecResult> {
  return new Promise((resolve, reject) => {
    let stdout = '';
    let stderr = '';
    let code: number | null = null;
    let ending = false;
    const timer = setTimeout(() => {
      channel.close();
      reject(new Error(`the TV did not finish within ${options.timeoutMs ?? EXEC_TIMEOUT_MS} ms`));
    }, options.timeoutMs ?? EXEC_TIMEOUT_MS);
    const end = () => {
      if (ending) return;
      ending = true;
      channel.end();
      setTimeout(() => channel.close(), END_GRACE_MS).unref();
    };
    const split = lineSplitter((line) => {
      if (options.onLine?.(line) === true) end();
    });
    channel.on('data', (chunk: Buffer) => {
      const text = chunk.toString('utf8');
      if (stdout.length + text.length <= MAX_OUTPUT_BYTES) stdout += text;
      split(text);
    });
    channel.stderr.on('data', (chunk: Buffer) => {
      if (stderr.length + chunk.length <= MAX_OUTPUT_BYTES) stderr += chunk.toString('utf8');
    });
    channel.on('exit', (status: number | null) => {
      code = status;
    });
    channel.on('close', () => {
      clearTimeout(timer);
      resolve({ code, stdout, stderr });
    });
    options.stdin?.on('error', reject).pipe(channel);
  });
}

function sftpWrite(
  sftp: SFTPWrapper,
  source: Readable,
  remotePath: string,
  onProgress: (bytes: number) => void,
): Promise<void> {
  return new Promise((resolve, reject) => {
    let sent = 0;
    const target = sftp.createWriteStream(remotePath);
    source.on('data', (chunk: Buffer) => {
      sent += chunk.length;
      onProgress(sent);
    });
    source.on('error', reject);
    target.on('error', reject);
    target.on('close', () => resolve());
    source.pipe(target);
  });
}

function session(client: Client): SshSession {
  return {
    exec: (command, options = {}) =>
      new Promise((resolve, reject) => {
        client.exec(command, (error, channel) => {
          if (error) reject(error);
          else runOnChannel(channel, options).then(resolve, reject);
        });
      }),
    sftpPut: (source, remotePath, onProgress = () => undefined) =>
      new Promise((resolve, reject) => {
        client.sftp((error, sftp) => {
          if (error) {
            reject(new SftpUnavailableError(error.message));
            return;
          }
          sftpWrite(sftp, source, remotePath, onProgress)
            .finally(() => sftp.end())
            .then(resolve, reject);
        });
      }),
    close: () => client.end(),
  };
}

/**
 * Logs in to a TV's Developer Mode SSH server with its key. The host key is
 * not checked: a TV has no stable one to pin.
 */
export async function openSshSession(
  target: SshTarget,
  timeoutMs = HANDSHAKE_TIMEOUT_MS,
): Promise<SshSession> {
  const socket = await openSocket(target);
  return new Promise((resolve, reject) => {
    const client = new ssh2.Client();
    client.once('ready', () => {
      client.removeAllListeners('error');
      client.on('error', () => undefined);
      resolve(session(client));
    });
    client.once('error', (error) => {
      client.destroy();
      reject(handshakeFailure(error));
    });
    try {
      client.connect({
        sock: socket,
        username: DEVELOPER_USER,
        privateKey: target.privateKey,
        readyTimeout: timeoutMs,
        algorithms: { kex: { append: LEGACY_KEY_EXCHANGES, prepend: [], remove: [] } },
      });
    } catch (error) {
      socket.destroy();
      reject(new SshConnectError('denied', error instanceof Error ? error.message : String(error)));
    }
  });
}
