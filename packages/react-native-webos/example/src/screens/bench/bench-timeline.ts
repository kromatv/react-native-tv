import { useEffect } from 'react';
import { report } from './bench-report.ts';

/** Whether the bench runs its fixed timeline and ignores the remote (`EXPO_PUBLIC_BENCH_TIMELINE=1`). */
export const TIMELINE = process.env.EXPO_PUBLIC_BENCH_TIMELINE === '1';

/** The intervals the list's focus can move at, in milliseconds. */
export const SPEEDS_MS = [200, 50] as const;

/** What the bench shows: the screen's index and the speed's index, -1 for still. */
export interface BenchState {
  readonly screen: number;
  readonly speed: number;
}

const PHASES = [
  { atMs: 0, name: 'idle', screen: 0, speed: -1 },
  { atMs: 20_000, name: 'scroll-200', screen: 0, speed: 0 },
  { atMs: 60_000, name: 'scroll-50', screen: 0, speed: 1 },
  { atMs: 100_000, name: 'animation', screen: 1, speed: -1 },
  { atMs: 130_000, name: 'done', screen: 1, speed: -1 },
] as const;

/** Moves the bench through its phases from mount, reporting each one, when `TIMELINE` is on. */
export function useBenchTimeline(apply: (state: BenchState) => void): void {
  useEffect(() => {
    if (!TIMELINE) return;
    const timers = PHASES.map(({ atMs, name, screen, speed }) =>
      setTimeout(() => {
        apply({ screen, speed });
        void report('phase', { name });
      }, atMs),
    );
    return () => {
      for (const timer of timers) clearTimeout(timer);
    };
  }, [apply]);
}
