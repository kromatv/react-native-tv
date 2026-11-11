import { describe, expect, it } from 'vitest';
import { painter } from '../paint.ts';
import { formatMemoryPeak, processMemoryMb } from './process-memory.ts';
import { idleSample } from './perf-sample.fixtures.ts';
import { summarizePerf } from './perf-summary.ts';

const plain = painter(false);

describe('processMemoryMb', () => {
  it('adds swap to PSS, and is null for a build that reports neither', () => {
    expect(processMemoryMb(idleSample({ pssMb: 140, swapMb: 2 }))).toBe(142);
    expect(processMemoryMb(idleSample({ pssMb: 140 }))).toBe(140);
    expect(processMemoryMb(idleSample())).toBeNull();
  });
});

describe('formatMemoryPeak', () => {
  it("gives the run's highest PSS and swap", () => {
    const samples = [
      idleSample({ pssMb: 120, swapMb: 0, memorySource: 'smaps_rollup' }),
      idleSample({ pssMb: 178, swapMb: 2, memorySource: 'smaps_rollup' }),
    ];

    expect(formatMemoryPeak(samples, plain)).toEqual([
      'memory peak 180.0 MB',
      '  PSS and swap; GPU memory, which the app cannot read, comes on top',
    ]);
  });

  it('names RSS when PSS was not readable, and stays silent without readings', () => {
    const fromStatus = formatMemoryPeak(
      [idleSample({ pssMb: 320, memorySource: 'status' })],
      plain,
    );

    expect(fromStatus[0]).toBe('memory peak 320.0 MB');
    expect(fromStatus[1]).toContain('RSS and swap, which count shared pages in full');
    expect(formatMemoryPeak([idleSample()], plain)).toEqual([]);
  });
});

describe('the pss + swap row', () => {
  it('summarizes the samples that report memory, and is left out when none does', () => {
    const rows = summarizePerf([
      idleSample({ pssMb: 100 }),
      idleSample(),
      idleSample({ pssMb: 140 }),
    ]);

    expect(rows.find((row) => row.label === 'pss + swap')).toMatchObject({ avg: 120, max: 140 });
    expect(summarizePerf([idleSample()]).some((row) => row.label === 'pss + swap')).toBe(false);
  });
});
