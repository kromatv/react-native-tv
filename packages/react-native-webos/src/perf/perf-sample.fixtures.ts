import type { PerfSample } from './perf-sample.ts';

/** A sample with every field at rest, for tests to override. */
export function idleSample(overrides: Partial<PerfSample> = {}): PerfSample {
  return {
    t: 1000,
    window: 1000,
    frames: 0,
    ticks: 60,
    hz: 60,
    dropped: 0,
    renderAvg: 0,
    renderMax: 0,
    mainBusy: 0,
    jsBusy: 0,
    jsWait: 0,
    jsLag: 0,
    commits: 0,
    commitAvg: 0,
    commitMax: 0,
    layoutMax: 0,
    mutations: 0,
    mountAvg: 0,
    mountMax: 0,
    mountLatencyMax: 0,
    mountTop: [],
    heapUsedMb: 12.5,
    heapSizeMb: 32,
    gcs: 3,
    cpu: 1,
    ...overrides,
  };
}
