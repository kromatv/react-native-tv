import { describe, expect, it } from 'vitest';
import { idleSample } from './perf-sample.fixtures.ts';
import { perfSampleOf, share } from './perf-sample.ts';

describe('perfSampleOf', () => {
  it('reads the sample a perf line carries', () => {
    const sample = idleSample({ frames: 58, dropped: 2 });

    expect(perfSampleOf({ level: 'debug', message: `perf ${JSON.stringify(sample)}` })).toEqual(
      sample,
    );
  });

  it('leaves an ordinary log line alone', () => {
    expect(perfSampleOf({ level: 'info', message: 'Running "main"' })).toBeNull();
  });

  it('refuses a perf line whose fields do not add up', () => {
    const broken = { ...idleSample(), frames: -1 };

    expect(perfSampleOf({ level: 'debug', message: `perf ${JSON.stringify(broken)}` })).toBeNull();
  });

  it('refuses a perf line that is not JSON', () => {
    expect(perfSampleOf({ level: 'debug', message: 'perf samples on' })).toBeNull();
  });
});

describe('share', () => {
  it('is the percentage of the window', () => {
    expect(share(250, 1000)).toBe(25);
  });

  it('is zero for an empty window', () => {
    expect(share(5, 0)).toBe(0);
  });
});

describe('perfSampleOf with a sample from before mountTop', () => {
  it('reads it with no component costs', () => {
    const { mountTop: _omitted, ...older } = idleSample();

    expect(
      perfSampleOf({ level: 'debug', message: `perf ${JSON.stringify(older)}` })?.mountTop,
    ).toEqual([]);
  });
});
