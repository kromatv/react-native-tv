import { createConnection, type Socket } from 'node:net';
import {
  commandName,
  encodePacket,
  HOST_MAX_DATA,
  PacketReader,
  PROTOCOL_VERSION,
  SDB_COMMAND,
  type SdbPacket,
} from './packet.ts';
import { SdbStream, type StreamTransport } from './sdb-stream.ts';

/** Where a TV's sdb daemon listens: its address and port 26101 unless told otherwise. */
export interface SdbAddress {
  readonly host: string;
  readonly port: number;
}

export const SDB_PORT = 26101;

const HOST_BANNER = 'host::\0';
const DEFAULT_TIMEOUT_MS = 10_000;

/** Why a connection failed before the TV sent its banner. */
export type ConnectFailure = 'unreachable' | 'refused' | 'silent';

/** A connection that never reached the handshake, with the reason the CLI explains. */
export class SdbConnectError extends Error {
  readonly reason: ConnectFailure;

  constructor(reason: ConnectFailure, message: string) {
    super(message);
    this.name = 'SdbConnectError';
    this.reason = reason;
  }
}

function dial({ host, port }: SdbAddress, timeoutMs: number): Promise<Socket> {
  return new Promise((resolve, reject) => {
    const socket = createConnection({ host, port });
    socket.setNoDelay(true);
    socket.setTimeout(timeoutMs);
    const fail = (reason: ConnectFailure, message: string) => {
      socket.destroy();
      reject(new SdbConnectError(reason, `sdb: ${message} ${host}:${port}`));
    };
    socket.once('timeout', () => fail('unreachable', 'timed out connecting to'));
    socket.once('error', (error: NodeJS.ErrnoException) => {
      if (error.code === 'ECONNRESET') fail('refused', 'the connection was reset by');
      else fail('unreachable', 'could not reach');
    });
    socket.once('connect', () => {
      socket.setTimeout(0);
      socket.removeAllListeners('timeout');
      socket.removeAllListeners('error');
      resolve(socket);
    });
  });
}

function bannerText(data: Buffer): string {
  let end = data.length;
  while (end > 0 && data[end - 1] === 0) end -= 1;
  return data.subarray(0, end).toString('utf8');
}

/**
 * A connection to one TV's sdb daemon. Each service runs on a stream of its
 * own, and several can be open at once on the one socket.
 */
export class SdbConnection implements StreamTransport {
  maxData = HOST_MAX_DATA;
  banner = '';

  private readonly reader = new PacketReader(HOST_MAX_DATA);
  private readonly streams = new Map<number, SdbStream>();
  private nextId = 1;
  private failure: Error | null = null;
  private onBanner: ((banner: string) => void) | null = null;
  private onHandshakeFailed: ((error: Error) => void) | null = null;

  private constructor(private readonly socket: Socket) {
    socket.on('data', (chunk) => this.receive(chunk));
    socket.on('error', (error) => this.fail(error));
    socket.on('close', () => this.fail(new Error('sdb: the TV closed the connection')));
  }

  /** Connects and completes the handshake, or throws an {@link SdbConnectError}. */
  static async open(address: SdbAddress, timeoutMs = DEFAULT_TIMEOUT_MS): Promise<SdbConnection> {
    const connection = new SdbConnection(await dial(address, timeoutMs));
    try {
      await connection.handshake(timeoutMs);
    } catch (error) {
      connection.close();
      throw error;
    }
    return connection;
  }

  async openStream(service: string, timeoutMs = DEFAULT_TIMEOUT_MS): Promise<SdbStream> {
    if (this.failure !== null) throw this.failure;
    const localId = this.nextId++;
    const stream = new SdbStream(service, localId, this);
    this.streams.set(localId, stream);
    const opened = stream.opened(timeoutMs);
    this.send(SDB_COMMAND.OPEN, localId, 0, Buffer.from(`${service}\0`, 'utf8'));
    await opened;
    return stream;
  }

  send(command: number, arg0: number, arg1: number, data: Buffer): void {
    if (!this.socket.destroyed) this.socket.write(encodePacket({ command, arg0, arg1, data }));
  }

  release(localId: number): void {
    this.streams.delete(localId);
  }

  close(): void {
    this.socket.destroy();
  }

  private handshake(timeoutMs: number): Promise<void> {
    return new Promise((resolve, reject) => {
      const timer = setTimeout(() => {
        reject(
          new SdbConnectError('silent', 'sdb: the TV accepted the connection but never answered'),
        );
      }, timeoutMs);
      const settle = (error?: Error) => {
        clearTimeout(timer);
        this.onBanner = null;
        this.onHandshakeFailed = null;
        if (error === undefined) resolve();
        else reject(error);
      };
      this.onBanner = (banner) => {
        this.banner = banner;
        settle();
      };
      this.onHandshakeFailed = () => {
        settle(
          new SdbConnectError('refused', 'sdb: the TV closed the connection before its banner'),
        );
      };
      this.send(SDB_COMMAND.CNXN, PROTOCOL_VERSION, HOST_MAX_DATA, Buffer.from(HOST_BANNER));
    });
  }

  private receive(chunk: Buffer): void {
    let packets: SdbPacket[];
    try {
      packets = this.reader.push(chunk);
    } catch (error) {
      this.fail(error instanceof Error ? error : new Error(String(error)));
      return;
    }
    for (const packet of packets) this.dispatch(packet);
  }

  private dispatch(packet: SdbPacket): void {
    if (packet.command === SDB_COMMAND.CNXN) {
      this.maxData = Math.min(HOST_MAX_DATA, packet.arg1 || HOST_MAX_DATA);
      this.onBanner?.(bannerText(packet.data));
      return;
    }
    const stream = this.streams.get(packet.arg1);
    if (stream === undefined) return;
    if (packet.command === SDB_COMMAND.OKAY) stream.onOkay(packet.arg0);
    else if (packet.command === SDB_COMMAND.WRTE) stream.onData(packet.data);
    else if (packet.command === SDB_COMMAND.CLSE) stream.onClose();
    else this.fail(new Error(`sdb: an unexpected ${commandName(packet.command)} from the TV`));
  }

  private fail(error: Error): void {
    if (this.failure !== null) return;
    this.failure = error;
    this.onHandshakeFailed?.(error);
    for (const stream of this.streams.values()) stream.onFail(error);
    this.streams.clear();
    this.socket.destroy();
  }
}
