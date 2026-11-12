import { inflateSync } from 'node:zlib';
import { describe, expect, it } from 'vitest';
import { encodePng } from './png.ts';

interface Chunk {
  readonly type: string;
  readonly data: Buffer;
}

function readChunks(png: Buffer): Chunk[] {
  const chunks: Chunk[] = [];
  for (let offset = 8; offset < png.length;) {
    const length = png.readUInt32BE(offset);
    chunks.push({
      type: png.toString('latin1', offset + 4, offset + 8),
      data: png.subarray(offset + 8, offset + 8 + length),
    });
    offset += 12 + length;
  }
  return chunks;
}

describe('encodePng', () => {
  it('writes a PNG with the frame size, RGBA, 8 bits', () => {
    const frame = { width: 3, height: 2, stride: 12, pixels: Buffer.alloc(24, 0xff) };

    const png = encodePng(frame);
    const [header] = readChunks(png);

    expect(png.subarray(0, 8)).toEqual(
      Buffer.from([0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a]),
    );
    expect(header?.type).toBe('IHDR');
    expect(header?.data.readUInt32BE(0)).toBe(3);
    expect(header?.data.readUInt32BE(4)).toBe(2);
    expect([...(header?.data.subarray(8) ?? [])]).toEqual([8, 6, 0, 0, 0]);
  });

  it('turns premultiplied BGRA into straight RGBA and skips the row padding', () => {
    const pixels = Buffer.from([
      0x10, 0x20, 0x30, 0xff, 0x40, 0x00, 0x20, 0x80, 0xee, 0xee, 0xee, 0xee,
    ]);
    const frame = { width: 2, height: 1, stride: 12, pixels };

    const idat = readChunks(encodePng(frame)).find((chunk) => chunk.type === 'IDAT');

    expect([...inflateSync(idat?.data ?? Buffer.alloc(0))]).toEqual([
      0, 0x30, 0x20, 0x10, 0xff, 0x40, 0x00, 0x80, 0x80,
    ]);
  });

  it('ends with IEND and a CRC that matches the chunk', () => {
    const png = encodePng({ width: 1, height: 1, stride: 4, pixels: Buffer.from([0, 0, 0, 0]) });

    expect(png.subarray(-12)).toEqual(
      Buffer.from([0, 0, 0, 0, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82]),
    );
  });
});
