import { mkdir, writeFile } from 'node:fs/promises';
import { networkInterfaces } from 'node:os';
import { dirname } from 'node:path';
import { setTimeout as delay } from 'node:timers/promises';
import { untilInterrupted } from '../cli/log-level-option.ts';
import type { Reporter } from '../cli/reporter.ts';
import { pickLanAddress } from '../logs/lan-address.ts';
import { listenForLogs } from '../logs/log-server.ts';
import { PerfListener } from './perf-listener.ts';
import type { PerfChoice } from './perf-options.ts';
import type { PerfSample } from './perf-sample.ts';
import {
  formatPerfSummary,
  formatSlowestToMount,
  slowestToMount,
  summarizePerf,
} from './perf-summary.ts';

const QUIET_NOTE_MS = 10_000;
const FIRST_SAMPLE_GRACE_MS = 30_000;

/** Where a perf run listens and what it does with what it hears. */
export interface PerfWatch extends PerfChoice {
  readonly port: number;
  readonly device: string | null;
  readonly inUseHint: string;
}

/**
 * Listens on a development build's log port, asks every app that connects for
 * a perf sample a second, prints each, and at Ctrl+C or after the duration
 * prints the run's summary and saves the samples when asked.
 */
export async function watchPerf(reporter: Reporter, watch: PerfWatch): Promise<PerfSample[]> {
  const listener = new PerfListener({
    paint: reporter.paint,
    minimum: watch.minimum,
    device: watch.device,
  });
  const server = await listenForLogs(watch.port, listener, {
    inUseHint: watch.inUseHint,
    peer: watch.device,
  });
  const address = pickLanAddress(networkInterfaces(), watch.device) ?? 'this computer';
  const until =
    watch.durationMs === null
      ? 'Press Ctrl+C to stop'
      : `Stopping ${watch.durationMs / 1000} s after the first sample`;
  reporter.line(`Listening on ${address}:${server.port} for perf samples. ${until}.`);
  const quiet = setTimeout(() => {
    if (listener.samples.length > 0) return;
    reporter.note(
      `No sample yet. A development build packaged with port ${server.port} sends them once it connects; one built before rn-tizen perf existed never does.`,
    );
  }, QUIET_NOTE_MS);
  const stopped = untilInterrupted();
  if (watch.durationMs !== null) {
    const durationMs = watch.durationMs;
    const sampled = listener.firstSample.then(() => delay(durationMs, undefined, { ref: false }));
    const fallback = delay(durationMs + FIRST_SAMPLE_GRACE_MS, undefined, { ref: false });
    await Promise.race([stopped, sampled, fallback]);
  } else {
    await stopped;
  }
  clearTimeout(quiet);
  await server.close();
  const samples = listener.samples;
  if (watch.out !== null) {
    await mkdir(dirname(watch.out), { recursive: true });
    await writeFile(watch.out, samples.map((sample) => `${JSON.stringify(sample)}\n`).join(''));
  }
  reporter.line();
  const saved = watch.out === null ? '' : `, saved to ${watch.out}`;
  reporter.line(`${samples.length} samples${saved}`);
  const lines = [
    ...formatPerfSummary(summarizePerf(samples), reporter.paint),
    ...formatSlowestToMount(slowestToMount(samples), reporter.paint),
  ];
  for (const line of lines) reporter.line(line);
  return samples;
}
