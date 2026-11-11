import { crc32, deflateSync } from 'node:zlib';
import { encodePng } from './png-image.ts';

const SIGNATURE = [0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a];

function chunk(type: string, body: Uint8Array): Buffer {
  const typed = Buffer.concat([Buffer.from(type, 'latin1'), body]);
  const length = Buffer.alloc(4);
  length.writeUInt32BE(body.byteLength);
  const crc = Buffer.alloc(4);
  crc.writeUInt32BE(crc32(typed));
  return Buffer.concat([length, typed, crc]);
}

interface RawPng {
  readonly width: number;
  readonly height: number;
  readonly bitDepth: number;
  readonly colorType: number;
  readonly interlaced?: boolean;
  /** Each row: its filter type, then its bytes as filtered. */
  readonly rows: readonly (readonly number[])[];
  readonly palette?: readonly number[];
  readonly transparency?: readonly number[];
}

/** A PNG written from scanlines as given, so a test chooses the color type, the depth and the filters. */
export function rawPng(png: RawPng): Buffer {
  const header = Buffer.alloc(13);
  header.writeUInt32BE(png.width, 0);
  header.writeUInt32BE(png.height, 4);
  header.set([png.bitDepth, png.colorType, 0, 0, png.interlaced === true ? 1 : 0], 8);
  return Buffer.concat([
    Buffer.from(SIGNATURE),
    chunk('IHDR', header),
    ...(png.palette === undefined ? [] : [chunk('PLTE', Buffer.from(png.palette))]),
    ...(png.transparency === undefined ? [] : [chunk('tRNS', Buffer.from(png.transparency))]),
    chunk('IDAT', deflateSync(Buffer.from(png.rows.flat()))),
    chunk('IEND', Buffer.alloc(0)),
  ]);
}

/** A PNG of `width` by `height` in one RGBA color. */
export function plainPng(
  width: number,
  height: number,
  color: readonly number[] = [10, 20, 30, 255],
): Buffer {
  const pixels = new Uint8Array(width * height * 4);
  for (let at = 0; at < pixels.byteLength; at += 4) pixels.set(color, at);
  return encodePng({ width, height, pixels });
}
