import { SDB_COMMAND } from './packet.ts';

/** What a stream needs from the connection it runs on. */
export interface StreamTransport {
  readonly maxData: number;
  send(command: number, arg0: number, arg1: number, data: Buffer): void;
  release(localId: number): void;
}

interface Waiter<T> {
  readonly resolve: (value: T) => void;
  readonly reject: (error: Error) => void;
}

const EMPTY = Buffer.alloc(0);
const DEFAULT_TIMEOUT_MS = 30_000;

function withDeadline<T>(promise: Promise<T>, timeoutMs: number, message: string): Promise<T> {
  let timer: NodeJS.Timeout | undefined;
  const expiry = new Promise<never>((_, reject) => {
    timer = setTimeout(() => reject(new Error(message)), timeoutMs);
  });
  return Promise.race([promise, expiry]).finally(() => clearTimeout(timer));
}

/**
 * One service opened on a connection (`capability:`, `shell:...`, `sync:`).
 * Every write waits for the TV's OKAY before the next one goes out.
 */
export class SdbStream {
  private remoteId = 0;
  private readonly pending: Buffer[] = [];
  private readonly readers: Waiter<Buffer | null>[] = [];
  private readonly acks: Waiter<void>[] = [];
  private opening: Waiter<void> | null = null;
  private ended = false;
  private failure: Error | null = null;

  constructor(
    readonly service: string,
    readonly localId: number,
    private readonly transport: StreamTransport,
  ) {}

  opened(timeoutMs: number): Promise<void> {
    const accepted = new Promise<void>((resolve, reject) => {
      this.opening = { resolve, reject };
    });
    return withDeadline(accepted, timeoutMs, `sdb: the TV never answered ${this.service}`);
  }

  onOkay(remoteId: number): void {
    if (this.remoteId === 0) {
      this.remoteId = remoteId;
      this.opening?.resolve();
      this.opening = null;
      return;
    }
    this.acks.shift()?.resolve();
  }

  onData(data: Buffer): void {
    this.transport.send(SDB_COMMAND.OKAY, this.localId, this.remoteId, EMPTY);
    const reader = this.readers.shift();
    if (reader === undefined) this.pending.push(data);
    else reader.resolve(data);
  }

  onClose(): void {
    if (this.ended) return;
    this.ended = true;
    if (this.remoteId !== 0) this.transport.send(SDB_COMMAND.CLSE, 0, this.remoteId, EMPTY);
    this.transport.release(this.localId);
    this.opening?.reject(new Error(`sdb: the TV refused ${this.service}`));
    this.opening = null;
    for (const reader of this.readers.splice(0)) reader.resolve(null);
    const closed = new Error(`sdb: the TV closed ${this.service} during a write`);
    for (const ack of this.acks.splice(0)) ack.reject(closed);
  }

  onFail(error: Error): void {
    this.failure = error;
    this.ended = true;
    this.opening?.reject(error);
    this.opening = null;
    for (const reader of this.readers.splice(0)) reader.reject(error);
    for (const ack of this.acks.splice(0)) ack.reject(error);
  }

  /** Sends `data` in packets as large as the TV takes, each acknowledged before the next. */
  async write(data: Buffer, timeoutMs = DEFAULT_TIMEOUT_MS): Promise<void> {
    if (data.length === 0) return;
    if (this.failure !== null) throw this.failure;
    if (this.ended) throw new Error(`sdb: the TV closed ${this.service}`);
    const acknowledged = new Promise<void>((resolve, reject) => {
      this.acks.push({ resolve, reject });
    });
    const chunk = data.subarray(0, this.transport.maxData);
    this.transport.send(SDB_COMMAND.WRTE, this.localId, this.remoteId, chunk);
    await withDeadline(acknowledged, timeoutMs, `sdb: the TV stopped reading ${this.service}`);
    return this.write(data.subarray(chunk.length), timeoutMs);
  }

  /** The next payload the TV wrote, or null once it closed the stream. */
  read(timeoutMs = DEFAULT_TIMEOUT_MS): Promise<Buffer | null> {
    if (this.failure !== null) return Promise.reject(this.failure);
    const buffered = this.pending.shift();
    if (buffered !== undefined) return Promise.resolve(buffered);
    if (this.ended) return Promise.resolve(null);
    const next = new Promise<Buffer | null>((resolve, reject) => {
      this.readers.push({ resolve, reject });
    });
    return withDeadline(next, timeoutMs, `sdb: ${this.service} went quiet`);
  }

  /** Each payload the TV writes, until it closes the stream. */
  chunks(timeoutMs = DEFAULT_TIMEOUT_MS): AsyncIterable<Buffer> {
    return {
      [Symbol.asyncIterator]: () => ({
        next: async (): Promise<IteratorResult<Buffer, undefined>> => {
          const chunk = await this.read(timeoutMs);
          return chunk === null ? { done: true, value: undefined } : { done: false, value: chunk };
        },
      }),
    };
  }

  /** Everything the TV writes until it closes the stream, cut at `limit` bytes. */
  async drain(limit: number, timeoutMs = DEFAULT_TIMEOUT_MS): Promise<Buffer> {
    const chunks: Buffer[] = [];
    let size = 0;
    for await (const chunk of this.chunks(timeoutMs)) {
      chunks.push(chunk);
      size += chunk.length;
      if (size >= limit) break;
    }
    return Buffer.concat(chunks).subarray(0, limit);
  }

  close(): void {
    this.onClose();
  }
}
