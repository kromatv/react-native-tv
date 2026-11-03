import { z } from 'zod';
import type { LogEntry } from '../logs/log-line.ts';

const PREFIX = 'perf ';

const count = z.number().int().nonnegative();
const millis = z.number().nonnegative();

const mountCostSchema = z.object({ name: z.string().max(128), ms: millis, mutations: count });

const perfSampleSchema = z.object({
  t: millis,
  window: millis,
  frames: count,
  ticks: count,
  hz: millis,
  dropped: count,
  renderAvg: millis,
  renderMax: millis,
  mainBusy: millis,
  jsBusy: millis,
  jsWait: millis,
  jsLag: millis,
  commits: count,
  commitAvg: millis,
  commitMax: millis,
  layoutMax: millis,
  mutations: count,
  mountAvg: millis,
  mountMax: millis,
  mountLatencyMax: millis,
  mountTop: z.array(mountCostSchema).max(16).default([]),
  heapUsedMb: millis,
  heapSizeMb: millis,
  gcs: count,
  cpu: millis,
});

/**
 * One second of a development build's main loop and JavaScript thread, as its
 * `PerfMonitor` logs it. Times are milliseconds, `cpu` percent of one core.
 */
export type PerfSample = z.infer<typeof perfSampleSchema>;

/** What mounting one kind of component cost the main loop in a sample. */
export type MountCost = z.infer<typeof mountCostSchema>;

function parseJson(text: string): unknown {
  try {
    return JSON.parse(text);
  } catch {
    return null;
  }
}

/** The sample a log entry carries, or null when it is an ordinary log line. */
export function perfSampleOf(entry: LogEntry): PerfSample | null {
  if (!entry.message.startsWith(PREFIX)) return null;
  const parsed = perfSampleSchema.safeParse(parseJson(entry.message.slice(PREFIX.length)));
  return parsed.success ? parsed.data : null;
}

/** The share of `window` that `busy` milliseconds take, in percent. */
export function share(busy: number, window: number): number {
  return window > 0 ? (busy / window) * 100 : 0;
}
