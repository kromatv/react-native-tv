import { describe, expect, it } from 'vitest';
import { fitImage, resizeImage } from './scale-image.ts';

function image(width: number, height: number, pixel: (x: number, y: number) => readonly number[]) {
  const pixels = new Uint8Array(width * height * 4);
  for (let y = 0; y < height; y += 1) {
    for (let x = 0; x < width; x += 1) pixels.set(pixel(x, y), (y * width + x) * 4);
  }
  return { width, height, pixels };
}

describe('resizeImage', () => {
  it('averages the pixels a smaller pixel covers', () => {
    const checker = image(4, 2, (x) => (x % 2 === 0 ? [0, 0, 0, 255] : [200, 100, 50, 255]));

    expect([...resizeImage(checker, 2, 1).pixels]).toEqual([100, 50, 25, 255, 100, 50, 25, 255]);
  });

  it('weights a color by its alpha, so a transparent neighbour does not darken it', () => {
    const edge = image(2, 1, (x) => (x === 0 ? [255, 0, 0, 255] : [0, 0, 0, 0]));

    expect([...resizeImage(edge, 1, 1).pixels]).toEqual([255, 0, 0, 128]);
  });

  it('weights a pixel covered in part by the part covered', () => {
    const ramp = image(3, 1, (x) => [x * 90, 0, 0, 255]);

    expect([...resizeImage(ramp, 2, 1).pixels]).toEqual([30, 0, 0, 255, 150, 0, 0, 255]);
  });

  it('repeats pixels when the image grows', () => {
    const two = image(2, 1, (x) => [x * 255, 0, 0, 255]);

    expect(resizeImage(two, 4, 1).pixels.filter((_, index) => index % 4 === 0)).toEqual(
      Uint8Array.from([0, 0, 255, 255]),
    );
  });
});

describe('fitImage', () => {
  it('fills the canvas with a square image', () => {
    const fitted = fitImage(
      image(4, 4, () => [1, 2, 3, 255]),
      2,
      2,
    );

    expect(fitted).toMatchObject({ width: 2, height: 2 });
    expect([...fitted.pixels.subarray(0, 4)]).toEqual([1, 2, 3, 255]);
  });

  it('centers a wide image on transparency instead of stretching it', () => {
    const fitted = fitImage(
      image(4, 2, () => [9, 9, 9, 255]),
      4,
      4,
    );
    const alphaByRow = [0, 1, 2, 3].map((row) => fitted.pixels[(row * 4 + 1) * 4 + 3]);

    expect(alphaByRow).toEqual([0, 255, 255, 0]);
  });
});
