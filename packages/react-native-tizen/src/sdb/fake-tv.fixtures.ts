import { createServer, type Socket } from 'node:net';
import { encodePacket, PacketReader, SDB_COMMAND, type SdbPacket } from './packet.ts';

/** The capability lines GQ75LS03DAUXZG (2024, Tizen 9.0) answered with. */
export const TV_CAPABILITIES = [
  'secure_protocol:enabled',
  'intershell_support:disabled',
  'filesync_support:pushpull',
  'cpu_arch:armv7',
  'core_abi:arm_32',
  'sdk_toolpath:/home/owner/share/tmp/sdk_tools',
  'profile_name:tv',
  'vendor_name:Samsung',
  'can_launch:tv-samsung',
  'device_name:Tizen',
  'platform_version:9.0',
  'appcmd_support:disabled',
  'architecture:32',
  '',
].join('\n');

export const TV_BANNER = 'device::GQ75LS03DAUXZG::0';
const TV_MAX_DATA = 0x3ff9c;
const EMPTY = Buffer.alloc(0);

export function capabilityPayload(text: string): Buffer {
  const body = Buffer.from(text, 'utf8');
  const length = Buffer.alloc(2);
  length.writeUInt16LE(body.length, 0);
  return Buffer.concat([length, body]);
}

/** One service the fake TV accepted, and how to answer it. */
export interface FakeStream {
  readonly service: string;
  write(text: string | Buffer): void;
  close(): void;
}

/** Answers a service as it opens; what it returns sees each payload the host writes. */
export type ServiceHandler = (stream: FakeStream) => ((data: Buffer) => void) | void;

export interface FakeTv {
  readonly port: number;
  readonly packets: SdbPacket[];
  readonly opened: string[];
  close(): Promise<void>;
}

interface FakeTvOptions {
  /** `reset` drops every connection at once, as the TV does while another host holds it; `silent` never answers. */
  readonly handshake?: 'banner' | 'reset' | 'silent';
  readonly services: Readonly<Record<string, ServiceHandler>>;
}

function handlerFor(services: FakeTvOptions['services'], service: string) {
  const prefix = Object.keys(services).find((name) => service.startsWith(name));
  return prefix === undefined ? undefined : services[prefix];
}

/** A local TCP server that answers as a TV's sdb daemon, one handler per service prefix. */
export async function fakeTv({ handshake = 'banner', services }: FakeTvOptions): Promise<FakeTv> {
  const packets: SdbPacket[] = [];
  const opened: string[] = [];
  const sockets: Socket[] = [];
  const server = createServer((socket) => {
    sockets.push(socket);
    socket.on('error', () => undefined);
    if (handshake === 'reset') {
      socket.resetAndDestroy();
      return;
    }
    const reader = new PacketReader();
    const listeners = new Map<number, (data: Buffer) => void>();
    let nextId = 0x700;
    const send = (command: number, arg0: number, arg1: number, data = EMPTY) =>
      socket.write(encodePacket({ command, arg0, arg1, data }));
    socket.on('data', (chunk) => {
      for (const packet of reader.push(chunk)) {
        packets.push(packet);
        if (packet.command === SDB_COMMAND.CNXN && handshake === 'silent') continue;
        if (packet.command === SDB_COMMAND.CNXN) {
          send(SDB_COMMAND.CNXN, 0x02000000, TV_MAX_DATA, Buffer.from(`${TV_BANNER}\0`));
        } else if (packet.command === SDB_COMMAND.OPEN) {
          const service = packet.data.subarray(0, packet.data.indexOf(0)).toString('utf8');
          opened.push(service);
          const handler = handlerFor(services, service);
          if (handler === undefined) {
            send(SDB_COMMAND.CLSE, 0, packet.arg0);
            continue;
          }
          const tvId = nextId++;
          const hostId = packet.arg0;
          send(SDB_COMMAND.OKAY, tvId, hostId);
          const listener = handler({
            service,
            write: (text) => send(SDB_COMMAND.WRTE, tvId, hostId, Buffer.from(text)),
            close: () => send(SDB_COMMAND.CLSE, 0, hostId),
          });
          if (listener !== undefined) listeners.set(tvId, listener);
        } else if (packet.command === SDB_COMMAND.WRTE) {
          send(SDB_COMMAND.OKAY, packet.arg1, packet.arg0);
          listeners.get(packet.arg1)?.(packet.data);
        }
      }
    });
  });
  await new Promise<void>((resolve) => server.listen(0, '127.0.0.1', resolve));
  const address = server.address();
  if (address === null || typeof address === 'string') throw new Error('the fake TV got no port');
  return {
    port: address.port,
    packets,
    opened,
    close: () =>
      new Promise((resolve) => {
        for (const socket of sockets) socket.destroy();
        server.close(() => resolve());
      }),
  };
}

/** A sync request as the host sent it. DATA and SEND carry a payload, the rest a value. */
export interface SyncRequest {
  readonly id: string;
  readonly value: number;
  readonly payload: Buffer;
}

/** The complete sync requests at the start of `bytes`. */
export function syncRequests(bytes: Buffer): SyncRequest[] {
  const requests: SyncRequest[] = [];
  let at = 0;
  while (at + 8 <= bytes.length) {
    const id = bytes.subarray(at, at + 4).toString('latin1');
    const value = bytes.readUInt32LE(at + 4);
    const size = id === 'SEND' || id === 'DATA' ? value : 0;
    if (at + 8 + size > bytes.length) break;
    requests.push({ id, value, payload: bytes.subarray(at + 8, at + 8 + size) });
    at += 8 + size;
  }
  return requests;
}

/** A `sync:` handler that answers `reply` once the host sends DONE, and keeps every request. */
export function acceptPush(
  requests: SyncRequest[],
  reply = Buffer.from('OKAY\0\0\0\0', 'latin1'),
): ServiceHandler {
  return (stream) => {
    let bytes = Buffer.alloc(0);
    return (data) => {
      bytes = Buffer.concat([bytes, data]);
      const parsed = syncRequests(bytes);
      if (parsed.at(-1)?.id !== 'DONE' || requests.some((request) => request.id === 'DONE')) return;
      requests.push(...parsed);
      stream.write(reply);
    };
  };
}
