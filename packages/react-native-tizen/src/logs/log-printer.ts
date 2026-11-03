import type { Paint } from '../paint.ts';
import { formatLogEntry, isAtLeast, type LogEntry, type LogLevel } from './log-line.ts';
import type { LogListener } from './log-server.ts';

const MAX_HELD_LINES = 2000;

interface LogPrinterOptions {
  readonly paint: Paint;
  readonly transform?: (entry: LogEntry) => LogEntry;
  readonly minimum: LogLevel;
  readonly held: boolean;
  readonly out?: NodeJS.WriteStream;
}

/**
 * Prints the entries a TV streams, quieter levels filtered out. A held printer
 * keeps what arrives until {@link LogPrinter.release}, so logs never interleave
 * with build steps still drawing.
 */
export class LogPrinter implements LogListener {
  private readonly options: LogPrinterOptions;
  private readonly out: NodeJS.WriteStream;
  private held: boolean;
  private readonly pending: string[] = [];

  constructor(options: LogPrinterOptions) {
    this.options = options;
    this.out = options.out ?? process.stdout;
    this.held = options.held;
  }

  onEntry(entry: LogEntry): void {
    if (!isAtLeast(entry.level, this.options.minimum)) return;
    const shown = this.options.transform?.(entry) ?? entry;
    this.print(formatLogEntry(shown, new Date(), this.options.paint));
  }

  onConnect(from: string): void {
    this.print(this.options.paint('gray', `The app connected from ${from}`));
  }

  onDisconnect(from: string): void {
    this.print(this.options.paint('gray', `The app at ${from} disconnected`));
  }

  release(): void {
    this.held = false;
    for (const line of this.pending.splice(0)) this.out.write(`${line}\n`);
  }

  private print(line: string): void {
    if (!this.held) {
      this.out.write(`${line}\n`);
      return;
    }
    this.pending.push(line);
    if (this.pending.length > MAX_HELD_LINES) this.pending.shift();
  }
}
