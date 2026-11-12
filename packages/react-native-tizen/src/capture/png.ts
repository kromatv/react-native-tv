import { deflateSync } from 'node:zlib';
import type { ScreenFrame } from './capture-protocol.ts';

const SIGNATURE = Buffer.from([0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a]);
const BIT_DEPTH = 8;
const COLOR_TYPE_RGBA = 6;
const FILTER_NONE = 0;
const OPAQUE = 255;

const CRC_TABLE = Uint32Array.from({ length: 256 }, (_, index) => {
  let crc = index;
  for (let bit = 0; bit < 8; bit += 1) crc = crc & 1 ? 0xedb88320 ^ (crc >>> 1) : crc >>> 1;
  return crc >>> 0;
});

function crc32(bytes: Buffer): number {
  let crc = 0xffffffff;
  for (const byte of bytes) crc = (CRC_TABLE[(crc ^ byte) & 0xff] ?? 0) ^ (crc >>> 8);
  return (crc ^ 0xffffffff) >>> 0;
}

function chunk(type: string, data: Buffer): Buffer {
  const typed = Buffer.concat([Buffer.from(type, 'latin1'), data]);
  const length = Buffer.alloc(4);
  length.writeUInt32BE(data.length);
  const crc = Buffer.alloc(4);
  crc.writeUInt32BE(crc32(typed));
  return Buffer.concat([length, typed, crc]);
}

function unpremultiply(channel: number, alpha: number): number {
  if (alpha === OPAQUE) return channel;
  if (alpha === 0) return 0;
  return Math.min(OPAQUE, Math.round((channel * OPAQUE) / alpha));
}

function scanlines(frame: ScreenFrame): Buffer {
  const rowBytes = frame.width * 4;
  const out = Buffer.alloc((rowBytes + 1) * frame.height);
  const { pixels } = frame;
  for (let row = 0; row < frame.height; row += 1) {
    const target = row * (rowBytes + 1);
    out[target] = FILTER_NONE;
    for (let column = 0; column < frame.width; column += 1) {
      const from = row * frame.stride + column * 4;
      const to = target + 1 + column * 4;
      const alpha = pixels[from + 3] ?? 0;
      out[to] = unpremultiply(pixels[from + 2] ?? 0, alpha);
      out[to + 1] = unpremultiply(pixels[from + 1] ?? 0, alpha);
      out[to + 2] = unpremultiply(pixels[from] ?? 0, alpha);
      out[to + 3] = alpha;
    }
  }
  return out;
}

/** Encodes a captured screen as an 8-bit RGBA PNG, its alpha straightened. */
export function encodePng(frame: ScreenFrame): Buffer {
  const header = Buffer.alloc(13);
  header.writeUInt32BE(frame.width, 0);
  header.writeUInt32BE(frame.height, 4);
  header.writeUInt8(BIT_DEPTH, 8);
  header.writeUInt8(COLOR_TYPE_RGBA, 9);
  return Buffer.concat([
    SIGNATURE,
    chunk('IHDR', header),
    chunk('IDAT', deflateSync(scanlines(frame), { level: 6 })),
    chunk('IEND', Buffer.alloc(0)),
  ]);
}
