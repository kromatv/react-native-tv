import { crc32, deflateSync } from 'node:zlib';
import { PNG_SIGNATURE, type RgbaImage } from './png-image.ts';

const COLOR_TYPE_RGBA = 6;

function chunk(type: string, body: Uint8Array): Buffer {
  const typed = Buffer.concat([Buffer.from(type, 'latin1'), body]);
  const framed = Buffer.alloc(8 + typed.byteLength);
  framed.writeUInt32BE(body.byteLength, 0);
  typed.copy(framed, 4);
  framed.writeUInt32BE(crc32(typed), 4 + typed.byteLength);
  return framed;
}

/** Encodes an image as an 8-bit RGBA PNG. */
export function encodePng(image: RgbaImage): Buffer {
  const header = Buffer.alloc(13);
  header.writeUInt32BE(image.width, 0);
  header.writeUInt32BE(image.height, 4);
  header.writeUInt8(8, 8);
  header.writeUInt8(COLOR_TYPE_RGBA, 9);
  const rowBytes = image.width * 4;
  const rows = Buffer.alloc((rowBytes + 1) * image.height);
  for (let row = 0; row < image.height; row += 1) {
    rows.set(image.pixels.subarray(row * rowBytes, (row + 1) * rowBytes), row * (rowBytes + 1) + 1);
  }
  return Buffer.concat([
    PNG_SIGNATURE,
    chunk('IHDR', header),
    chunk('IDAT', deflateSync(rows)),
    chunk('IEND', new Uint8Array()),
  ]);
}
