import { describe, expect, it } from 'vitest';
import { painter } from '../paint.ts';
import { idleSample } from './perf-sample.fixtures.ts';
import { formatPerfSample, framePeriod } from './perf-format.ts';

const NINE_AM = new Date(2026, 10, 3, 9, 0, 5);
const plain = painter(false);

describe('formatPerfSample', () => {
  it('says idle when nothing drew, committed or ran', () => {
    expect(formatPerfSample(idleSample(), NINE_AM, plain)).toBe(
      '09:00:05  idle  heap  12.5/   32 MB  cpu   1%',
    );
  });

  it('prints frames against the display rate, then each thread', () => {
    const line = formatPerfSample(
      idleSample({
        frames: 58,
        dropped: 2,
        renderAvg: 3.1,
        renderMax: 9.4,
        mainBusy: 210,
        jsBusy: 350,
        jsWait: 40,
        jsLag: 2,
        commits: 3,
        commitAvg: 12.1,
        commitMax: 30.2,
        mountAvg: 4,
        mountMax: 9,
        cpu: 45,
      }),
      NINE_AM,
      plain,
    );

    expect(line).toBe(
      '09:00:05  fps  58/60  drop   2  render   3.1/   9.4 ms  main  21%  js  35%  wait   4%  lag    2.0 ms  commit   3x  12.1/  30.2 ms  mount   4.0/   9.0 ms  heap  12.5/   32 MB  cpu  45%',
    );
  });

  it('marks a render slower than a frame', () => {
    const paint = (format: unknown, text: string) => (format === 'red' ? `<${text}>` : text);

    const line = formatPerfSample(idleSample({ frames: 1, renderMax: 20 }), NINE_AM, paint);

    expect(line).toContain('<render   0.0/  20.0 ms>');
  });
});

describe('framePeriod', () => {
  it('is the display period', () => {
    expect(framePeriod(idleSample({ hz: 50 }))).toBe(20);
  });

  it('assumes 60 Hz before the display reported a rate', () => {
    expect(framePeriod(idleSample({ hz: 0 }))).toBeCloseTo(16.67, 2);
  });
});
