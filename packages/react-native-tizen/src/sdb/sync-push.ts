import { createReadStream } from 'node:fs';
import { stat } from 'node:fs/promises';
import type { SdbConnection } from './sdb-connection.ts';
import type { SdbStream } from './sdb-stream.ts';

export const SYNC_HEADER_BYTES = 8;
export const SYNC_DATA_MAX = 64 * 1024;
/** The mode sdb sends with a pushed package: `0755`, written in decimal. */
export const PUSH_MODE = 0o755;

const REPLY_TIMEOUT_MS = 120_000;
const MAX_FAIL_MESSAGE = 4096;

/** A sync request: four ASCII letters and a uint32 little-endian, then `payload`. */
export function syncRequest(id: string, value: number, payload: Buffer = Buffer.alloc(0)): Buffer {
  const header = Buffer.alloc(SYNC_HEADER_BYTES);
  header.write(id, 0, 4, 'ascii');
  header.writeUInt32LE(value >>> 0, 4);
  return Buffer.concat([header, payload]);
}

export function sendRequest(remotePath: string, mode = PUSH_MODE): Buffer {
  const target = Buffer.from(`${remotePath},${mode}`, 'utf8');
  return syncRequest('SEND', target.length, target);
}

/** Packs sync requests into writes as large as the TV takes, as sdb does. */
class PackedWriter {
  private queued: Buffer[] = [];
  private size = 0;

  constructor(
    private readonly stream: SdbStream,
    private readonly packetBytes: number,
  ) {}

  async add(bytes: Buffer): Promise<void> {
    this.queued.push(bytes);
    this.size += bytes.length;
    if (this.size < this.packetBytes) return;
    const joined = Buffer.concat(this.queued);
    const whole = joined.length - (joined.length % this.packetBytes);
    await this.stream.write(joined.subarray(0, whole), REPLY_TIMEOUT_MS);
    this.queued = [joined.subarray(whole)];
    this.size = joined.length - whole;
  }

  async flush(): Promise<void> {
    if (this.size > 0) await this.stream.write(Buffer.concat(this.queued), REPLY_TIMEOUT_MS);
    this.queued = [];
    this.size = 0;
  }
}

async function readReply(stream: SdbStream): Promise<void> {
  let reply = Buffer.alloc(0);
  for (;;) {
    const chunk = await stream.read(REPLY_TIMEOUT_MS);
    if (chunk === null) throw new Error('sdb: the TV closed the push before answering');
    reply = Buffer.concat([reply, chunk]);
    if (reply.length < SYNC_HEADER_BYTES) continue;
    const id = reply.subarray(0, 4).toString('ascii');
    if (id === 'OKAY') return;
    if (id !== 'FAIL') throw new Error(`sdb: the TV answered the push with ${JSON.stringify(id)}`);
    const length = Math.min(reply.readUInt32LE(4), MAX_FAIL_MESSAGE);
    if (reply.length < SYNC_HEADER_BYTES + length) continue;
    const message = reply.subarray(SYNC_HEADER_BYTES, SYNC_HEADER_BYTES + length).toString('utf8');
    throw new Error(`sdb: the TV refused the file: ${message}`);
  }
}

/** Copies a local file to `remotePath` on the TV over the `sync:` service. */
export async function pushFile(
  connection: SdbConnection,
  localPath: string,
  remotePath: string,
  onProgress: (sent: number, total: number) => void = () => undefined,
): Promise<void> {
  const { size: total, mtimeMs } = await stat(localPath);
  const stream = await connection.openStream('sync:');
  try {
    const writer = new PackedWriter(stream, connection.maxData);
    await writer.add(sendRequest(remotePath));
    let sent = 0;
    for await (const chunk of createReadStream(localPath, { highWaterMark: SYNC_DATA_MAX })) {
      if (!Buffer.isBuffer(chunk)) throw new Error(`sdb: ${localPath} did not read as bytes`);
      await writer.add(syncRequest('DATA', chunk.length, chunk));
      sent += chunk.length;
      onProgress(sent, total);
    }
    await writer.add(syncRequest('DONE', Math.floor(mtimeMs / 1000)));
    await writer.flush();
    await readReply(stream);
    await stream.write(syncRequest('QUIT', 0)).catch(() => undefined);
  } finally {
    stream.close();
  }
}
