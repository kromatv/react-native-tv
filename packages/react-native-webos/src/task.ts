import type { RunOptions } from './exec.ts';

/**
 * What a long-running step reports through: the commands it runs, which the
 * CLI shows or hides depending on `--verbose`, and how far along it is.
 */
export interface Task {
  run(command: readonly string[], options?: Omit<RunOptions, 'quiet'>): Promise<string>;
  progress(detail: string): void;
}
