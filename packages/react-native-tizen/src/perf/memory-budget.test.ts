import { describe, expect, it } from 'vitest';
import { painter } from '../paint.ts';
import { formatMemoryBudget, processMemoryMb } from './memory-budget.ts';
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

describe('formatMemoryBudget', () => {
  it("says how far the run's peak is from the UHD and FHD budgets", () => {
    const samples = [
      idleSample({ pssMb: 120, swapMb: 0, memorySource: 'smaps_rollup' }),
      idleSample({ pssMb: 178, swapMb: 2, memorySource: 'smaps_rollup' }),
    ];

    expect(formatMemoryBudget(samples, plain)).toEqual([
      'memory peak 180.0 MB: 200.0 MB under 380 MB, the UHD budget; 120.0 MB under 300 MB, the FHD one',
      '  PSS and swap; the budget also counts GPU and GEM memory, which the app cannot read',
    ]);
  });

  it('says when the peak is over, names RSS when PSS was not readable, and stays silent without readings', () => {
    const over = formatMemoryBudget([idleSample({ pssMb: 320, memorySource: 'status' })], plain);

    expect(over[0]).toContain('60.0 MB under 380 MB, the UHD budget; 20.0 MB over 300 MB');
    expect(over[1]).toContain('RSS and swap, which count shared pages in full');
    expect(formatMemoryBudget([idleSample()], plain)).toEqual([]);
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
