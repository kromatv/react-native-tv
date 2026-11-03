import { spawn } from 'node:child_process';

/** How {@link run} executes a command. */
export interface RunOptions {
  readonly cwd?: string;
  readonly env?: NodeJS.ProcessEnv;
  readonly quiet?: boolean;
  readonly onLine?: (line: string) => void;
}

/** Thrown by {@link run} when a command exits non-zero; carries what it printed. */
export class CommandError extends Error {
  readonly command: readonly string[];
  readonly exitCode: number | null;
  readonly output: string;

  constructor(command: readonly string[], exitCode: number | null, output: string) {
    super(`${command.join(' ')} exited with ${exitCode ?? 'a signal'}`);
    this.name = 'CommandError';
    this.command = command;
    this.exitCode = exitCode;
    this.output = output;
  }
}

/** Thrown by {@link run} when the program to run is not installed. */
export class MissingProgramError extends Error {
  readonly program: string;

  constructor(program: string) {
    super(`${program} is not installed or not on PATH`);
    this.name = 'MissingProgramError';
    this.program = program;
  }
}

function lineSplitter(onLine: (line: string) => void): (chunk: Buffer) => void {
  let pending = '';
  return (chunk) => {
    const lines = (pending + chunk.toString('utf8')).split(/\r?\n/);
    pending = lines.pop() ?? '';
    for (const line of lines) onLine(line);
  };
}

/**
 * Runs a command to completion and returns its combined output. It streams to
 * the terminal unless `quiet`, in which case the output only surfaces through
 * the {@link CommandError} a failure throws. `onLine` sees every complete line.
 */
export function run(command: readonly string[], options: RunOptions = {}): Promise<string> {
  const [program, ...args] = command;
  if (program === undefined) return Promise.reject(new Error('run() needs a command'));
  return new Promise((resolve, reject) => {
    const child = spawn(program, args, {
      cwd: options.cwd,
      env: options.env ?? process.env,
      stdio: ['ignore', 'pipe', 'pipe'],
    });
    const chunks: Buffer[] = [];
    const split = options.onLine ? lineSplitter(options.onLine) : null;
    const collect = (sink: NodeJS.WriteStream) => (chunk: Buffer) => {
      chunks.push(chunk);
      split?.(chunk);
      if (!options.quiet) sink.write(chunk);
    };
    child.stdout.on('data', collect(process.stdout));
    child.stderr.on('data', collect(process.stderr));
    child.on('error', (error: NodeJS.ErrnoException) => {
      reject(error.code === 'ENOENT' ? new MissingProgramError(program) : error);
    });
    child.on('close', (exitCode) => {
      const output = Buffer.concat(chunks).toString('utf8');
      if (exitCode === 0) resolve(output);
      else reject(new CommandError(command, exitCode, output));
    });
  });
}

/** The last `count` non-blank lines of a command's output. */
export function lastLines(output: string, count: number): string[] {
  return output
    .split(/\r?\n/)
    .filter((line) => line.trim() !== '')
    .slice(-count);
}
