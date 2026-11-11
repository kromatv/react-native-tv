import { describe, expect, it } from 'vitest';
import { checksum, encodePacket, HEADER_BYTES, PacketReader, SDB_COMMAND } from './packet.ts';

const CONNECT = {
  command: SDB_COMMAND.CNXN,
  arg0: 0x00100000,
  arg1: 0x40000,
  data: Buffer.from('host::\0'),
};

describe('encodePacket', () => {
  it('writes the handshake sdb 4.2.36 sent the TV, byte for byte', () => {
    const header = encodePacket(CONNECT).subarray(0, HEADER_BYTES);

    expect(header.toString('hex')).toBe('434e584e00001000000004000700000032020000bcb1a7b1');
  });

  it('sums the payload bytes for the checksum', () => {
    expect(checksum(Buffer.from([0xff, 0xff, 0x02]))).toBe(0x200);
  });
});

describe('PacketReader', () => {
  it('reassembles a packet split across chunks', () => {
    const bytes = encodePacket(CONNECT);
    const reader = new PacketReader();

    expect(reader.push(bytes.subarray(0, 10))).toEqual([]);
    expect(reader.push(bytes.subarray(10, 27))).toEqual([]);
    expect(reader.push(bytes.subarray(27))).toEqual([CONNECT]);
  });

  it('cuts several packets out of one chunk', () => {
    const okay = { command: SDB_COMMAND.OKAY, arg0: 0x753, arg1: 1, data: Buffer.alloc(0) };
    const bytes = Buffer.concat([encodePacket(CONNECT), encodePacket(okay)]);

    expect(new PacketReader().push(bytes)).toEqual([CONNECT, okay]);
  });

  it('refuses a header whose magic does not match its command', () => {
    const bytes = encodePacket(CONNECT);
    bytes.writeUInt32LE(0, 20);

    expect(() => new PacketReader().push(bytes)).toThrow('malformed CNXN');
  });

  it('refuses a payload larger than the host accepts', () => {
    const bytes = encodePacket({ ...CONNECT, data: Buffer.alloc(32) });

    expect(() => new PacketReader(16).push(bytes)).toThrow('malformed');
  });
});
