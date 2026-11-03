import { formatLogEntry, isAtLeast, type LogEntry, type LogLevel } from '../logs/log-line.ts';
import type { LogListener } from '../logs/log-server.ts';
import type { Paint } from '../paint.ts';
import { formatPerfSample } from './perf-format.ts';
import { perfSampleOf, type PerfSample } from './perf-sample.ts';

interface PerfListenerOptions {
  readonly paint: Paint;
  readonly minimum: LogLevel;
  readonly device: string | null;
  readonly out?: NodeJS.WritableStream;
  readonly now?: () => Date;
}

/**
 * Asks every development build that connects for perf samples, prints each
 * one as it arrives and keeps them for the summary. Log lines at `minimum` or
 * louder are printed between them; with a `device`, other TVs are ignored.
 */
export class PerfListener implements LogListener {
  readonly wantsPerf = true;
  readonly samples: PerfSample[] = [];
  /** Settles when the first sample arrives. */
  readonly firstSample: Promise<void>;
  private readonly options: PerfListenerOptions;
  private markFirstSample: () => void = () => undefined;

  constructor(options: PerfListenerOptions) {
    this.options = options;
    this.firstSample = new Promise((resolve) => {
      this.markFirstSample = resolve;
    });
  }

  onEntry(entry: LogEntry, from?: string): void {
    if (!this.accepts(from)) return;
    const time = this.options.now?.() ?? new Date();
    const sample = perfSampleOf(entry);
    if (sample !== null) {
      this.samples.push(sample);
      this.markFirstSample();
      this.print(formatPerfSample(sample, time, this.options.paint));
      return;
    }
    if (isAtLeast(entry.level, this.options.minimum)) {
      this.print(formatLogEntry(entry, time, this.options.paint));
    }
  }

  onConnect(from: string): void {
    if (this.accepts(from))
      this.print(this.options.paint('gray', `The app connected from ${from}`));
  }

  onDisconnect(from: string): void {
    if (this.accepts(from)) {
      this.print(this.options.paint('gray', `The app at ${from} disconnected`));
    }
  }

  private accepts(from: string | undefined): boolean {
    return this.options.device === null || from === undefined || from === this.options.device;
  }

  private print(line: string): void {
    (this.options.out ?? process.stdout).write(`${line}\n`);
  }
}
