/** adb's four-letter packet commands, read as little-endian integers. */
export const SDB_COMMAND = {
  CNXN: 0x4e584e43,
  OPEN: 0x4e45504f,
  OKAY: 0x59414b4f,
  CLSE: 0x45534c43,
  WRTE: 0x45545257,
} as const;

export const HEADER_BYTES = 24;
export const PROTOCOL_VERSION = 0x00100000;
export const HOST_MAX_DATA = 256 * 1024;

/** One sdb packet: a command, its two arguments and its payload. */
export interface SdbPacket {
  readonly command: number;
  readonly arg0: number;
  readonly arg1: number;
  readonly data: Buffer;
}

const NAMES = new Map<number, string>(
  Object.entries(SDB_COMMAND).map(([name, value]) => [value, name]),
);

export function commandName(command: number): string {
  return NAMES.get(command) ?? `0x${command.toString(16).padStart(8, '0')}`;
}

/** The header's payload check: the sum of the payload's bytes, modulo 2^32. */
export function checksum(data: Buffer): number {
  let total = 0;
  for (const byte of data) total = (total + byte) >>> 0;
  return total;
}

export function encodePacket({ command, arg0, arg1, data }: SdbPacket): Buffer {
  const header = Buffer.alloc(HEADER_BYTES);
  header.writeUInt32LE(command >>> 0, 0);
  header.writeUInt32LE(arg0 >>> 0, 4);
  header.writeUInt32LE(arg1 >>> 0, 8);
  header.writeUInt32LE(data.length, 12);
  header.writeUInt32LE(checksum(data), 16);
  header.writeUInt32LE((command ^ 0xffffffff) >>> 0, 20);
  return data.length === 0 ? header : Buffer.concat([header, data]);
}

/**
 * Cuts packets out of a byte stream. Throws on a bad magic or an oversized
 * payload: past such a header every byte is misframed.
 */
export class PacketReader {
  private buffer: Buffer = Buffer.alloc(0);

  constructor(private readonly maxData: number = HOST_MAX_DATA) {}

  push(chunk: Buffer): SdbPacket[] {
    this.buffer = this.buffer.length === 0 ? chunk : Buffer.concat([this.buffer, chunk]);
    const packets: SdbPacket[] = [];
    while (this.buffer.length >= HEADER_BYTES) {
      const command = this.buffer.readUInt32LE(0);
      const length = this.buffer.readUInt32LE(12);
      const magic = this.buffer.readUInt32LE(20);
      if (magic !== (command ^ 0xffffffff) >>> 0 || length > this.maxData) {
        throw new Error(`sdb: a malformed ${commandName(command)} packet from the TV`);
      }
      if (this.buffer.length < HEADER_BYTES + length) break;
      const data = this.buffer.subarray(HEADER_BYTES, HEADER_BYTES + length);
      packets.push({
        command,
        arg0: this.buffer.readUInt32LE(4),
        arg1: this.buffer.readUInt32LE(8),
        data,
      });
      this.buffer = this.buffer.subarray(HEADER_BYTES + length);
    }
    return packets;
  }
}
