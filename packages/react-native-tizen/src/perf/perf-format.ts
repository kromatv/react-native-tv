import type { Paint } from '../paint.ts';
import { share, type PerfSample } from './perf-sample.ts';

const TIME_WIDTH = 8;
const LAG_WARNING_MS = 100;
const BUSY_WARNING_PERCENT = 90;

function fixed(value: number, width: number, digits = 1): string {
  return value.toFixed(digits).padStart(width);
}

function percent(value: number): string {
  return `${Math.round(value)}%`.padStart(4);
}

function isIdle(sample: PerfSample): boolean {
  return sample.frames === 0 && sample.commits === 0 && sample.jsBusy < 1;
}

/** The frame period the display ticks at, in milliseconds, 60 Hz when unknown. */
export function framePeriod(sample: PerfSample): number {
  return 1000 / (sample.hz > 0 ? sample.hz : 60);
}

function redAbove(paint: Paint, value: number, limit: number, text: string): string {
  return value > limit ? paint('red', text) : text;
}

/**
 * One sample as one line: frames drawn against the display rate, dropped
 * frames, Evas render time (average/worst), main loop busy, the JavaScript
 * thread's busy share, its wait on the main loop and its queue delay, Fabric
 * commits and mounts (average/worst), the Hermes heap and the CPU.
 */
export function formatPerfSample(sample: PerfSample, time: Date, paint: Paint): string {
  const clock = paint('gray', time.toTimeString().slice(0, TIME_WIDTH));
  const memory = `heap ${fixed(sample.heapUsedMb, 5)}/${fixed(sample.heapSizeMb, 5, 0)} MB`;
  const cpu = `cpu ${percent(sample.cpu)}`;
  if (isIdle(sample)) return [clock, paint('gray', 'idle'), memory, cpu].join('  ');
  const jsBusy = share(sample.jsBusy, sample.window);
  const parts = [
    clock,
    `fps ${String(sample.frames).padStart(3)}/${Math.round(sample.hz)}`,
    redAbove(paint, sample.dropped, 0, `drop ${String(sample.dropped).padStart(3)}`),
    redAbove(
      paint,
      sample.renderMax,
      framePeriod(sample),
      `render ${fixed(sample.renderAvg, 5)}/${fixed(sample.renderMax, 6)} ms`,
    ),
    `main ${percent(share(sample.mainBusy, sample.window))}`,
    redAbove(paint, jsBusy, BUSY_WARNING_PERCENT, `js ${percent(jsBusy)}`),
    `wait ${percent(share(sample.jsWait, sample.window))}`,
    redAbove(paint, sample.jsLag, LAG_WARNING_MS, `lag ${fixed(sample.jsLag, 6)} ms`),
    `commit ${String(sample.commits).padStart(3)}x ${fixed(sample.commitAvg, 5)}/${fixed(sample.commitMax, 6)} ms`,
    `mount ${fixed(sample.mountAvg, 5)}/${fixed(sample.mountMax, 6)} ms`,
    memory,
    cpu,
  ];
  return parts.join('  ');
}
