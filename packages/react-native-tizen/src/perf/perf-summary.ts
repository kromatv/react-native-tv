import { processMemoryMb } from './memory-budget.ts';
import type { Paint } from '../paint.ts';
import { share, type MountCost, type PerfSample } from './perf-sample.ts';

/** One measurement over a run: its average, 95th percentile and worst. */
export interface PerfSummaryRow {
  readonly label: string;
  readonly unit: string;
  readonly avg: number;
  readonly p95: number;
  readonly max: number;
  readonly total?: number;
}

const LABEL_WIDTH = 16;
const COLUMN_WIDTH = 9;

function percentile(values: readonly number[], fraction: number): number {
  if (values.length === 0) return 0;
  const sorted = [...values].sort((a, b) => a - b);
  const rank = Math.ceil(fraction * sorted.length) - 1;
  return sorted[Math.min(sorted.length - 1, Math.max(0, rank))] ?? 0;
}

function sum(values: readonly number[]): number {
  return values.reduce((total, value) => total + value, 0);
}

function row(label: string, unit: string, values: readonly number[], avg?: number): PerfSummaryRow {
  return {
    label,
    unit,
    avg: avg ?? (values.length === 0 ? 0 : sum(values) / values.length),
    p95: percentile(values, 0.95),
    max: values.length === 0 ? 0 : Math.max(...values),
  };
}

function weighted(
  samples: readonly PerfSample[],
  value: (s: PerfSample) => number,
  weight: (s: PerfSample) => number,
): number {
  const weights = sum(samples.map(weight));
  return weights === 0 ? 0 : sum(samples.map((s) => value(s) * weight(s))) / weights;
}

function perSecond(samples: readonly PerfSample[], value: (s: PerfSample) => number): number[] {
  return samples.map((s) => (s.window > 0 ? (value(s) * 1000) / s.window : 0));
}

function counted(
  label: string,
  samples: readonly PerfSample[],
  value: (s: PerfSample) => number,
): PerfSummaryRow {
  return { ...row(label, '/s', perSecond(samples, value)), total: sum(samples.map(value)) };
}

/** Every measurement of a run, averaged across its samples. */
export function summarizePerf(samples: readonly PerfSample[]): PerfSummaryRow[] {
  const drawing = samples.filter((s) => s.frames > 0);
  const committing = samples.filter((s) => s.commits > 0);
  return [
    counted('frames drawn', samples, (s) => s.frames),
    counted('frames dropped', samples, (s) => s.dropped),
    row(
      'render',
      'ms',
      drawing.map((s) => s.renderMax),
      weighted(
        drawing,
        (s) => s.renderAvg,
        (s) => s.frames,
      ),
    ),
    row(
      'main loop busy',
      '%',
      samples.map((s) => share(s.mainBusy, s.window)),
    ),
    row(
      'js busy',
      '%',
      samples.map((s) => share(s.jsBusy, s.window)),
    ),
    row(
      'js wait on main',
      '%',
      samples.map((s) => share(s.jsWait, s.window)),
    ),
    row(
      'js lag',
      'ms',
      samples.map((s) => s.jsLag),
    ),
    counted('commits', samples, (s) => s.commits),
    row(
      'commit',
      'ms',
      committing.map((s) => s.commitMax),
      weighted(
        committing,
        (s) => s.commitAvg,
        (s) => s.commits,
      ),
    ),
    row(
      'mount',
      'ms',
      committing.map((s) => s.mountMax),
      weighted(
        committing,
        (s) => s.mountAvg,
        (s) => s.commits,
      ),
    ),
    row(
      'mount latency',
      'ms',
      committing.map((s) => s.mountLatencyMax),
    ),
    row(
      'heap used',
      'MB',
      samples.map((s) => s.heapUsedMb),
    ),
    ...memoryRow(samples),
    row(
      'cpu',
      '%',
      samples.map((s) => s.cpu),
    ),
  ];
}

function memoryRow(samples: readonly PerfSample[]): PerfSummaryRow[] {
  const readings = samples.flatMap((s) => {
    const memory = processMemoryMb(s);
    return memory === null ? [] : [memory];
  });
  return readings.length === 0 ? [] : [row('pss + swap', 'MB', readings)];
}

function cell(value: number): string {
  return value.toFixed(1).padStart(COLUMN_WIDTH);
}

/** The summary as a table: one row per measurement, avg, p95 and max columns. */
export function formatPerfSummary(rows: readonly PerfSummaryRow[], paint: Paint): string[] {
  const header = paint(
    'gray',
    `${''.padEnd(LABEL_WIDTH)}${'avg'.padStart(COLUMN_WIDTH)}${'p95'.padStart(COLUMN_WIDTH)}${'max'.padStart(COLUMN_WIDTH)}`,
  );
  const lines = rows.map((r) => {
    const total = r.total === undefined ? '' : paint('gray', `  total ${r.total}`);
    return `${r.label.padEnd(LABEL_WIDTH)}${cell(r.avg)}${cell(r.p95)}${cell(r.max)} ${r.unit}${total}`;
  });
  return [header, ...lines];
}

const SLOWEST_COMPONENTS = 5;

/** The components whose mutations cost the main loop most over a run, slowest first. */
export function slowestToMount(samples: readonly PerfSample[]): MountCost[] {
  const totals = new Map<string, MountCost>();
  for (const cost of samples.flatMap((s) => s.mountTop)) {
    const total = totals.get(cost.name) ?? { name: cost.name, ms: 0, mutations: 0 };
    totals.set(cost.name, {
      name: cost.name,
      ms: total.ms + cost.ms,
      mutations: total.mutations + cost.mutations,
    });
  }
  return [...totals.values()].sort((a, b) => b.ms - a.ms).slice(0, SLOWEST_COMPONENTS);
}

/** The slowest components as lines: name, total time, mutations. */
export function formatSlowestToMount(costs: readonly MountCost[], paint: Paint): string[] {
  if (costs.length === 0) return [];
  return [
    paint('gray', 'slowest to mount'),
    ...costs.map((c) => {
      const mutations = paint('gray', `${c.mutations} mutations`);
      return `  ${c.name.padEnd(LABEL_WIDTH - 2)}${cell(c.ms)} ms  ${mutations}`;
    }),
  ];
}
