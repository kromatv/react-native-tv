import { describe, expect, it } from 'vitest';
import { encodePng } from './png-encode.ts';
import { decodePng, pngSize } from './png-image.ts';
import { plainPng, rawPng } from './png.fixtures.ts';

const pixels = (png: Uint8Array) => [...decodePng(png).pixels];

describe('encodePng and decodePng', () => {
  it('give back the pixels they were given', () => {
    const image = {
      width: 2,
      height: 2,
      pixels: Uint8Array.from([255, 0, 0, 255, 0, 255, 0, 128, 0, 0, 255, 0, 9, 8, 7, 6]),
    };
    const png = encodePng(image);

    expect(pngSize(png)).toEqual({ width: 2, height: 2 });
    expect(decodePng(png)).toEqual(image);
  });
});

describe('decodePng', () => {
  it('undoes each of the five row filters', () => {
    const png = rawPng({
      width: 2,
      height: 5,
      bitDepth: 8,
      colorType: 0,
      rows: [
        [0, 10, 20],
        [1, 10, 10],
        [2, 5, 5],
        [3, 10, 10],
        [4, 1, 2],
      ],
    });
    const gray = decodePng(png).pixels.filter((_, index) => index % 4 === 0);

    expect([...gray]).toEqual([10, 20, 10, 20, 15, 25, 17, 31, 18, 33]);
  });

  it('reads a palette with its transparency, at two bits a pixel', () => {
    const png = rawPng({
      width: 4,
      height: 1,
      bitDepth: 2,
      colorType: 3,
      palette: [255, 0, 0, 0, 255, 0, 0, 0, 255, 9, 9, 9],
      transparency: [255, 128, 0],
      rows: [[0, 0b00011011]],
    });

    expect(pixels(png)).toEqual([255, 0, 0, 255, 0, 255, 0, 128, 0, 0, 255, 0, 9, 9, 9, 255]);
  });

  it('spreads one-bit and sixteen-bit gray over 0 to 255, with alpha when there is some', () => {
    const bits = rawPng({
      width: 3,
      height: 1,
      bitDepth: 1,
      colorType: 0,
      rows: [[0, 0b10100000]],
    });
    const wide = rawPng({
      width: 1,
      height: 1,
      bitDepth: 16,
      colorType: 4,
      rows: [[0, 0x80, 0x12, 0x40, 0x34]],
    });

    expect(pixels(bits)).toEqual([255, 255, 255, 255, 0, 0, 0, 255, 255, 255, 255, 255]);
    expect(pixels(wide)).toEqual([0x80, 0x80, 0x80, 0x40]);
  });

  it('makes the one color an RGB file names transparent', () => {
    const png = rawPng({
      width: 2,
      height: 1,
      bitDepth: 8,
      colorType: 2,
      transparency: [0, 1, 0, 2, 0, 3],
      rows: [[0, 1, 2, 3, 3, 2, 1]],
    });

    expect(pixels(png)).toEqual([1, 2, 3, 0, 3, 2, 1, 255]);
  });

  it('refuses what it cannot read, saying why', () => {
    const png = plainPng(2, 2);
    const interlaced = rawPng({
      width: 1,
      height: 1,
      bitDepth: 8,
      colorType: 0,
      interlaced: true,
      rows: [[0, 1]],
    });
    const short = rawPng({ width: 2, height: 2, bitDepth: 8, colorType: 0, rows: [[0, 1, 2]] });
    const badFilter = rawPng({ width: 1, height: 1, bitDepth: 8, colorType: 0, rows: [[9, 1]] });

    expect(() => decodePng(Buffer.from('JFIF, not a PNG at all, but long enough'))).toThrow(
      'Not a PNG',
    );
    expect(() => decodePng(interlaced)).toThrow('interlaced');
    expect(() => decodePng(short)).toThrow('too few pixels');
    expect(() => decodePng(badFilter)).toThrow('unknown filter 9');
    expect(() => decodePng(png.subarray(0, png.byteLength - 20))).toThrow();
  });
});
