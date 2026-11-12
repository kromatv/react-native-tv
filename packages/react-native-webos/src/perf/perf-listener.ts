import { formatLogEntry, isAtLeast, type LogEntry, type LogLevel } from '../logs/log-line.ts';
import type { LogListener } from '../logs/log-server.ts';
import type { Paint } from '../paint.ts';
import { formatPerfSample } from './perf-format.ts';
import { perfSampleOf, type PerfSample } from './perf-sample.ts';

interface PerfListenerOptions {
  readonly paint: Paint;
  readonly minimum: LogLevel;
  readonly device: string | null;
  /** The package to read; with none, the first app that names one is followed. */
  readonly app?: string | null;
  readonly out?: NodeJS.WritableStream;
  readonly now?: () => Date;
}

/**
 * Asks every development build that connects for perf samples, prints each
 * one as it arrives and keeps them for the summary. Log lines at `minimum` or
 * louder are printed between them; with a `device`, other TVs are ignored.
 * Samples come from one app: the one asked for, else the first that connects.
 */
export class PerfListener implements LogListener {
  readonly wantsPerf = true;
  readonly samples: PerfSample[] = [];
  /** Settles when the first sample arrives. */
  readonly firstSample: Promise<void>;
  private readonly options: PerfListenerOptions;
  private markFirstSample: () => void = () => undefined;
  private followed: string | null;
  private readonly passedOver = new Set<string>();

  constructor(options: PerfListenerOptions) {
    this.options = options;
    this.followed = options.app ?? null;
    this.firstSample = new Promise((resolve) => {
      this.markFirstSample = resolve;
    });
  }

  /** The package the samples come from, null until an app that names one connects. */
  get app(): string | null {
    return this.followed;
  }

  onEntry(entry: LogEntry, from?: string, app: string | null = null): void {
    if (!this.accepts(from, app)) return;
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

  onConnect(from: string, app: string | null = null): void {
    if (this.accepts(from, app)) {
      this.print(this.options.paint('gray', `${app ?? 'The app'} connected from ${from}`));
    } else if (app !== null && !this.passedOver.has(app)) {
      this.passedOver.add(app);
      this.print(
        this.options.paint(
          'gray',
          `${app} connected too, not read: pass --package ${app} to read it`,
        ),
      );
    }
  }

  onDisconnect(from: string, app: string | null = null): void {
    if (this.accepts(from, app)) {
      this.print(this.options.paint('gray', `${app ?? 'The app'} at ${from} disconnected`));
    }
  }

  private accepts(from: string | undefined, app: string | null): boolean {
    const device = this.options.device;
    if (device !== null && from !== undefined && from !== device) return false;
    if (app === null) return true;
    this.followed ??= app;
    return app === this.followed;
  }

  private print(line: string): void {
    (this.options.out ?? process.stdout).write(`${line}\n`);
  }
}
