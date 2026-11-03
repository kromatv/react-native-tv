import { run } from '../exec.ts';
import { colorsEnabled, painter, type Paint } from '../paint.ts';
import type { Task } from '../task.ts';
import { formatDuration } from './format.ts';

const SPINNER = ['⠋', '⠙', '⠹', '⠸', '⠼', '⠴', '⠦', '⠧', '⠇', '⠏'];
const FRAME_MS = 80;
const CLEAR_LINE = '\r\u001b[2K';
const TITLE_WIDTH = 22;

interface ReporterOptions {
  readonly verbose: boolean;
  readonly out?: NodeJS.WriteStream;
  readonly env?: NodeJS.ProcessEnv;
}

/**
 * Prints a command's progress, one line per step. On a terminal a running step
 * animates in place; with `--verbose` the tools' own output streams under it.
 */
export class Reporter {
  readonly paint: Paint;
  readonly verbose: boolean;
  private readonly out: NodeJS.WriteStream;
  private readonly animated: boolean;
  private spinning = false;

  constructor(options: ReporterOptions) {
    const env = options.env ?? process.env;
    this.out = options.out ?? process.stdout;
    this.verbose = options.verbose;
    this.paint = painter(colorsEnabled(this.out, env));
    this.animated = this.out.isTTY === true && !options.verbose && !env.CI;
  }

  /** Prints a line; during an animated step it goes above the spinner, which redraws below it. */
  line(text = ''): void {
    this.out.write(`${this.spinning ? CLEAR_LINE : ''}${text}\n`);
  }

  field(label: string, value: string): void {
    this.line(`  ${this.paint('gray', label.padEnd(TITLE_WIDTH))} ${value}`);
  }

  note(text: string): void {
    this.line(`  ${this.paint('gray', text)}`);
  }

  /** A {@link Task} whose commands print only under `--verbose`. */
  task(progress: (detail: string) => void = () => undefined): Task {
    return {
      run: (command, options) => run(command, { ...options, quiet: !this.verbose }),
      progress,
    };
  }

  /** Runs one step, then prints it as done (with `summarize`'s text) or failed. */
  async step<T>(
    title: string,
    work: (task: Task) => Promise<T>,
    summarize?: (result: T) => string,
  ): Promise<T> {
    const started = performance.now();
    let detail = '';
    let frame = 0;
    const draw = () => {
      const spinner = this.paint('cyan', SPINNER[frame % SPINNER.length] ?? '');
      const elapsed = this.paint('gray', formatDuration(performance.now() - started));
      const progress = detail === '' ? '' : ` ${this.paint('gray', detail)}`;
      this.out.write(`${CLEAR_LINE}${spinner} ${title.padEnd(TITLE_WIDTH)}${progress} ${elapsed}`);
      frame += 1;
    };
    const task = this.task((text) => {
      detail = text;
    });
    if (this.verbose) this.line(`${this.paint('cyan', '›')} ${title}`);
    const ticker = this.animated ? setInterval(draw, FRAME_MS) : null;
    this.spinning = this.animated;
    if (this.animated) draw();
    const finish = (mark: string, summary: string) => {
      if (ticker !== null) clearInterval(ticker);
      this.spinning = false;
      const elapsed = this.paint('gray', formatDuration(performance.now() - started));
      const text = summary === '' ? '' : `${summary}  `;
      const line = `${mark} ${title.padEnd(TITLE_WIDTH)} ${text}${elapsed}`;
      this.out.write(`${this.animated ? CLEAR_LINE : ''}${line}\n`);
    };
    try {
      const result = await work(task);
      finish(this.paint('green', '✓'), summarize?.(result) ?? '');
      return result;
    } catch (error) {
      finish(this.paint('red', '✗'), '');
      throw error;
    }
  }
}
