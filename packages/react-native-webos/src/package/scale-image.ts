import type { RgbaImage } from './png-image.ts';

interface Span {
  readonly start: number;
  readonly weights: readonly number[];
}

function spans(source: number, target: number): Span[] {
  const step = source / target;
  return Array.from({ length: target }, (_, index) => {
    const from = index * step;
    const to = Math.min(source, from + step);
    const start = Math.floor(from);
    const weights: number[] = [];
    for (let cell = start; cell < to; cell += 1) {
      weights.push(Math.min(cell + 1, to) - Math.max(cell, from));
    }
    return { start, weights };
  });
}

/**
 * Resamples an image to `width` by `height` exactly, averaging the source
 * pixels each new pixel covers, colors weighted by their alpha so a
 * transparent edge does not darken.
 */
export function resizeImage(image: RgbaImage, width: number, height: number): RgbaImage {
  const columns = spans(image.width, width);
  const rows = spans(image.height, height);
  const pixels = new Uint8Array(width * height * 4);
  rows.forEach((rowSpan, y) => {
    columns.forEach((columnSpan, x) => {
      const sum = [0, 0, 0, 0];
      let area = 0;
      rowSpan.weights.forEach((rowWeight, rowOffset) => {
        columnSpan.weights.forEach((columnWeight, columnOffset) => {
          const at =
            ((rowSpan.start + rowOffset) * image.width + columnSpan.start + columnOffset) * 4;
          const weight = rowWeight * columnWeight;
          const alpha = (image.pixels[at + 3] ?? 0) * weight;
          sum[0] = (sum[0] ?? 0) + (image.pixels[at] ?? 0) * alpha;
          sum[1] = (sum[1] ?? 0) + (image.pixels[at + 1] ?? 0) * alpha;
          sum[2] = (sum[2] ?? 0) + (image.pixels[at + 2] ?? 0) * alpha;
          sum[3] = (sum[3] ?? 0) + alpha;
          area += weight;
        });
      });
      const alpha = sum[3] ?? 0;
      const color = (channel: number) =>
        alpha === 0 ? 0 : Math.round((sum[channel] ?? 0) / alpha);
      pixels.set([color(0), color(1), color(2), Math.round(alpha / area)], (y * width + x) * 4);
    });
  });
  return { width, height, pixels };
}

/**
 * Scales an image to fit a `width` by `height` canvas, keeping its
 * proportions and centering it on transparency.
 */
export function fitImage(image: RgbaImage, width: number, height: number): RgbaImage {
  const scale = Math.min(width / image.width, height / image.height);
  const fitted = {
    width: Math.max(1, Math.min(width, Math.round(image.width * scale))),
    height: Math.max(1, Math.min(height, Math.round(image.height * scale))),
  };
  const scaled = resizeImage(image, fitted.width, fitted.height);
  if (fitted.width === width && fitted.height === height) return scaled;
  const pixels = new Uint8Array(width * height * 4);
  const left = Math.floor((width - fitted.width) / 2);
  const top = Math.floor((height - fitted.height) / 2);
  for (let row = 0; row < fitted.height; row += 1) {
    const from = row * fitted.width * 4;
    pixels.set(
      scaled.pixels.subarray(from, from + fitted.width * 4),
      ((top + row) * width + left) * 4,
    );
  }
  return { width, height, pixels };
}
