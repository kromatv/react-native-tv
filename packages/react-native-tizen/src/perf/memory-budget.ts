import type { Paint } from '../paint.ts';
import type { PerfSample } from './perf-sample.ts';

/** Samsung's memory budget for an app: 380 MB on a UHD model, 300 MB on FHD, counting PSS, swap, GPU and GEM memory. */
export const MEMORY_BUDGET_MB = { uhd: 380, fhd: 300 } as const;

/** The process's PSS and swap in a sample, or null for a build that does not report them. */
export function processMemoryMb(sample: PerfSample): number | null {
  if (sample.pssMb === undefined) return null;
  return sample.pssMb + (sample.swapMb ?? 0);
}

/**
 * How far a run's highest PSS and swap came from Samsung's budget, as one line,
 * or none when no sample reported them. The app cannot read its GPU and GEM
 * memory, which the budget counts too.
 */
export function formatMemoryBudget(samples: readonly PerfSample[], paint: Paint): string[] {
  const readings = samples.flatMap((sample) => {
    const memory = processMemoryMb(sample);
    return memory === null ? [] : [{ memory, source: sample.memorySource }];
  });
  if (readings.length === 0) return [];
  const peak = Math.max(...readings.map((reading) => reading.memory));
  const against = (budget: number) => {
    const room = budget - peak;
    return room >= 0
      ? `${room.toFixed(1)} MB under ${budget} MB`
      : `${(-room).toFixed(1)} MB over ${budget} MB`;
  };
  const counted = readings.some((reading) => reading.source === 'status')
    ? 'RSS and swap, which count shared pages in full'
    : 'PSS and swap';
  const line = `memory peak ${peak.toFixed(1)} MB: ${against(MEMORY_BUDGET_MB.uhd)}, the UHD budget; ${against(MEMORY_BUDGET_MB.fhd)}, the FHD one`;
  const over = peak > MEMORY_BUDGET_MB.fhd;
  return [
    over ? paint('red', line) : line,
    paint(
      'gray',
      `  ${counted}; the budget also counts GPU and GEM memory, which the app cannot read`,
    ),
  ];
}
