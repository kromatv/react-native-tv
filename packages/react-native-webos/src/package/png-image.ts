import { inflateSync } from 'node:zlib';

/** An image as straight (not premultiplied) 8-bit RGBA, row after row. */
export interface RgbaImage {
  readonly width: number;
  readonly height: number;
  readonly pixels: Uint8Array;
}

/** The eight bytes every PNG starts with. */
export const PNG_SIGNATURE = Buffer.from([0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a]);
const MAX_PIXELS = 64 * 1024 * 1024;
const COLOR_GRAY = 0;
const COLOR_RGB = 2;
const COLOR_PALETTE = 3;
const COLOR_GRAY_ALPHA = 4;
const COLOR_RGBA = 6;
const CHANNELS: Readonly<Record<number, number>> = {
  [COLOR_GRAY]: 1,
  [COLOR_RGB]: 3,
  [COLOR_PALETTE]: 1,
  [COLOR_GRAY_ALPHA]: 2,
  [COLOR_RGBA]: 4,
};

interface PngHeader {
  readonly width: number;
  readonly height: number;
  readonly bitDepth: number;
  readonly colorType: number;
  readonly interlaced: boolean;
}

interface PngChunks {
  readonly header: PngHeader;
  readonly palette: Uint8Array;
  readonly transparency: Uint8Array | null;
  readonly data: Buffer;
}

function readHeader(png: Buffer): PngHeader {
  if (png.byteLength < 33 || !png.subarray(0, 8).equals(PNG_SIGNATURE))
    throw new Error('Not a PNG');
  if (png.toString('latin1', 12, 16) !== 'IHDR') throw new Error('The PNG has no header');
  return {
    width: png.readUInt32BE(16),
    height: png.readUInt32BE(20),
    bitDepth: png.readUInt8(24),
    colorType: png.readUInt8(25),
    interlaced: png.readUInt8(28) !== 0,
  };
}

/** The width and height a PNG's header declares. */
export function pngSize(png: Uint8Array): { readonly width: number; readonly height: number } {
  const { width, height } = readHeader(Buffer.from(png.buffer, png.byteOffset, png.byteLength));
  return { width, height };
}

function readChunks(png: Buffer): PngChunks {
  const header = readHeader(png);
  const data: Buffer[] = [];
  let palette: Uint8Array = new Uint8Array();
  let transparency: Uint8Array | null = null;
  for (let offset = 8; offset + 12 <= png.byteLength;) {
    const length = png.readUInt32BE(offset);
    const type = png.toString('latin1', offset + 4, offset + 8);
    const body = png.subarray(offset + 8, offset + 8 + length);
    if (body.byteLength < length) throw new Error('The PNG is cut short');
    if (type === 'PLTE') palette = body;
    else if (type === 'tRNS') transparency = body;
    else if (type === 'IDAT') data.push(body);
    else if (type === 'IEND') break;
    offset += 12 + length;
  }
  return { header, palette, transparency, data: Buffer.concat(data) };
}

function paeth(left: number, up: number, upLeft: number): number {
  const estimate = left + up - upLeft;
  const fromLeft = Math.abs(estimate - left);
  const fromUp = Math.abs(estimate - up);
  const fromUpLeft = Math.abs(estimate - upLeft);
  if (fromLeft <= fromUp && fromLeft <= fromUpLeft) return left;
  return fromUp <= fromUpLeft ? up : upLeft;
}

function predicted(filter: number, left: number, up: number, upLeft: number): number {
  const value = [0, left, up, (left + up) >> 1, paeth(left, up, upLeft)][filter];
  if (value === undefined) throw new Error(`The PNG uses an unknown filter ${filter}`);
  return value;
}

function unfilter(raw: Buffer, rowBytes: number, height: number, pixelBytes: number): Buffer {
  if (raw.byteLength < (rowBytes + 1) * height) throw new Error('The PNG holds too few pixels');
  const out = Buffer.alloc(rowBytes * (height + 1));
  for (let row = 0; row < height; row += 1) {
    const filter = raw[row * (rowBytes + 1)] ?? 0;
    const from = row * (rowBytes + 1) + 1;
    const to = (row + 1) * rowBytes;
    for (let column = 0; column < rowBytes; column += 1) {
      const left = column >= pixelBytes ? (out[to + column - pixelBytes] ?? 0) : 0;
      const up = out[to + column - rowBytes] ?? 0;
      const upLeft = column >= pixelBytes ? (out[to + column - rowBytes - pixelBytes] ?? 0) : 0;
      out[to + column] = ((raw[from + column] ?? 0) + predicted(filter, left, up, upLeft)) & 0xff;
    }
  }
  return out.subarray(rowBytes);
}

function sampleReader(rows: Buffer, rowBytes: number, bitDepth: number) {
  const scale = 255 / ((1 << Math.min(bitDepth, 8)) - 1);
  return (row: number, index: number, scaled = true): number => {
    const start = row * rowBytes;
    if (bitDepth === 16) return rows[start + index * 2] ?? 0;
    if (bitDepth === 8) return rows[start + index] ?? 0;
    const byte = rows[start + ((index * bitDepth) >> 3)] ?? 0;
    const shift = 8 - bitDepth - ((index * bitDepth) & 7);
    const value = (byte >> shift) & ((1 << bitDepth) - 1);
    return scaled ? Math.round(value * scale) : value;
  };
}

type Sample = ReturnType<typeof sampleReader>;
type Rgba = readonly [number, number, number, number];

function transparentColor(chunks: PngChunks): readonly number[] | null {
  const { transparency, header } = chunks;
  if (transparency === null || header.colorType === COLOR_PALETTE) return null;
  const view = Buffer.from(transparency.buffer, transparency.byteOffset, transparency.byteLength);
  const samples = header.colorType === COLOR_GRAY ? 1 : 3;
  if (view.byteLength < samples * 2) return null;
  const scale = 255 / ((1 << Math.min(header.bitDepth, 8)) - 1);
  return Array.from({ length: samples }, (_, index) => {
    const value = view.readUInt16BE(index * 2);
    return header.bitDepth === 16 ? value >> 8 : Math.round(value * scale);
  });
}

function pixelReader(chunks: PngChunks, sample: Sample): (row: number, at: number) => Rgba {
  const { palette, transparency } = chunks;
  switch (chunks.header.colorType) {
    case COLOR_PALETTE:
      return (row, at) => {
        const entry = sample(row, at, false) * 3;
        const alpha = transparency?.[entry / 3] ?? 255;
        return [palette[entry] ?? 0, palette[entry + 1] ?? 0, palette[entry + 2] ?? 0, alpha];
      };
    case COLOR_GRAY:
      return (row, at) => [sample(row, at), sample(row, at), sample(row, at), 255];
    case COLOR_GRAY_ALPHA:
      return (row, at) => [sample(row, at), sample(row, at), sample(row, at), sample(row, at + 1)];
    case COLOR_RGB:
      return (row, at) => [sample(row, at), sample(row, at + 1), sample(row, at + 2), 255];
    default:
      return (row, at) => [
        sample(row, at),
        sample(row, at + 1),
        sample(row, at + 2),
        sample(row, at + 3),
      ];
  }
}

/** Decodes a PNG of any color type and bit depth into 8-bit RGBA. Interlaced files are refused. */
export function decodePng(file: Uint8Array): RgbaImage {
  const chunks = readChunks(Buffer.from(file.buffer, file.byteOffset, file.byteLength));
  const { width, height, bitDepth, colorType, interlaced } = chunks.header;
  const channels = CHANNELS[colorType];
  if (channels === undefined || width === 0 || height === 0) throw new Error('Not a PNG image');
  if (interlaced) throw new Error('The PNG is interlaced: save it without interlacing');
  if (width * height > MAX_PIXELS) throw new Error('The PNG is too large');
  const rowBytes = Math.ceil((width * channels * bitDepth) / 8);
  const pixelBytes = Math.max(1, (channels * bitDepth) >> 3);
  const rows = unfilter(inflateSync(chunks.data), rowBytes, height, pixelBytes);
  const read = pixelReader(chunks, sampleReader(rows, rowBytes, bitDepth));
  const transparent = transparentColor(chunks);
  const pixels = new Uint8Array(width * height * 4);
  for (let row = 0; row < height; row += 1) {
    for (let column = 0; column < width; column += 1) {
      const pixel = read(row, column * channels);
      const keyed = transparent?.every((value, index) => value === pixel[index]) === true;
      pixels.set(keyed ? [pixel[0], pixel[1], pixel[2], 0] : pixel, (row * width + column) * 4);
    }
  }
  return { width, height, pixels };
}
