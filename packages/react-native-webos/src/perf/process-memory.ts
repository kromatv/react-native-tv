import type { Paint } from '../paint.ts';
import type { PerfSample } from './perf-sample.ts';

/** The process's PSS and swap in a sample, or null for a build that does not report them. */
export function processMemoryMb(sample: PerfSample): number | null {
  if (sample.pssMb === undefined) return null;
  return sample.pssMb + (sample.swapMb ?? 0);
}

/**
 * A run's highest PSS and swap as one line and what it counts, or nothing when
 * no sample reported them. LG publishes no memory budget for an app, so the
 * peak is compared with nothing.
 */
export function formatMemoryPeak(samples: readonly PerfSample[], paint: Paint): string[] {
  const readings = samples.flatMap((sample) => {
    const memory = processMemoryMb(sample);
    return memory === null ? [] : [{ memory, source: sample.memorySource }];
  });
  if (readings.length === 0) return [];
  const peak = Math.max(...readings.map((reading) => reading.memory));
  const counted = readings.some((reading) => reading.source === 'status')
    ? 'RSS and swap, which count shared pages in full'
    : 'PSS and swap';
  return [
    `memory peak ${peak.toFixed(1)} MB`,
    paint('gray', `  ${counted}; GPU memory, which the app cannot read, comes on top`),
  ];
}
