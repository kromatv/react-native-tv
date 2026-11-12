import { describe, expect, it } from 'vitest';
import { painter } from '../paint.ts';
import { idleSample } from './perf-sample.fixtures.ts';
import {
  formatPerfSummary,
  formatSlowestToMount,
  slowestToMount,
  summarizePerf,
} from './perf-summary.ts';

function rowOf(label: string, rows: ReturnType<typeof summarizePerf>) {
  const found = rows.find((row) => row.label === label);
  if (found === undefined) throw new Error(`no ${label} row`);
  return found;
}

describe('summarizePerf', () => {
  it('counts frames per second and in total', () => {
    const rows = summarizePerf([
      idleSample({ frames: 60, dropped: 0 }),
      idleSample({ frames: 30, dropped: 6, window: 500 }),
    ]);

    expect(rowOf('frames drawn', rows)).toMatchObject({ avg: 60, max: 60, total: 90 });
    expect(rowOf('frames dropped', rows)).toMatchObject({ avg: 6, max: 12, total: 6 });
  });

  it('weights the render average by the frames each second drew', () => {
    const rows = summarizePerf([
      idleSample({ frames: 30, renderAvg: 2, renderMax: 4 }),
      idleSample({ frames: 10, renderAvg: 10, renderMax: 25 }),
      idleSample(),
    ]);

    expect(rowOf('render', rows)).toMatchObject({ avg: 4, p95: 25, max: 25 });
  });

  it('reads thread load as a share of each window', () => {
    const rows = summarizePerf([idleSample({ jsBusy: 900 }), idleSample({ jsBusy: 100 })]);

    expect(rowOf('js busy', rows)).toMatchObject({ avg: 50, max: 90 });
  });

  it('leaves commit times out of seconds with no commit', () => {
    const rows = summarizePerf([
      idleSample({ commits: 2, commitAvg: 10, commitMax: 15 }),
      idleSample(),
    ]);

    expect(rowOf('commit', rows)).toMatchObject({ avg: 10, max: 15 });
  });

  it('reports zeros for a run with no sample', () => {
    expect(rowOf('cpu', summarizePerf([]))).toMatchObject({ avg: 0, p95: 0, max: 0 });
  });
});

describe('formatPerfSummary', () => {
  it('prints a header, then one aligned row per measurement', () => {
    const lines = formatPerfSummary(summarizePerf([idleSample({ cpu: 40 })]), painter(false));

    expect(lines[0]).toBe('                      avg      p95      max');
    expect(lines).toContain('cpu                  40.0     40.0     40.0 %');
    expect(lines).toContain('frames drawn          0.0      0.0      0.0 /s  total 0');
  });
});

describe('slowestToMount', () => {
  it('adds each component up across samples and keeps the slowest first', () => {
    const slowest = slowestToMount([
      idleSample({ mountTop: [{ name: 'Image', ms: 40, mutations: 10 }] }),
      idleSample({
        mountTop: [
          { name: 'Paragraph', ms: 90, mutations: 30 },
          { name: 'Image', ms: 60, mutations: 5 },
        ],
      }),
    ]);

    expect(slowest).toEqual([
      { name: 'Image', ms: 100, mutations: 15 },
      { name: 'Paragraph', ms: 90, mutations: 30 },
    ]);
  });

  it('prints nothing for a run that mounted nothing', () => {
    expect(formatSlowestToMount([], painter(false))).toEqual([]);
  });

  it('prints one line per component under a heading', () => {
    expect(
      formatSlowestToMount([{ name: 'Image', ms: 100, mutations: 15 }], painter(false)),
    ).toEqual(['slowest to mount', '  Image             100.0 ms  15 mutations']);
  });
});
