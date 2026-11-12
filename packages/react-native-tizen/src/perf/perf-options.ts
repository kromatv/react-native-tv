import { resolve } from 'node:path';
import { z } from 'zod';
import { UsageError, type OptionSpecs } from '../cli/command.ts';
import { parseLevel } from '../cli/log-level-option.ts';
import type { LogLevel } from '../logs/log-line.ts';

/** The options `rn-tizen perf` and `rn-tizen simulator perf` share. */
export const PERF_OPTIONS = {
  duration: {
    type: 'string',
    placeholder: '<seconds>',
    description:
      'perf: stop this many seconds after the first sample and print the summary (default: at Ctrl+C)',
  },
} as const satisfies OptionSpecs;

/** What a perf run reads from its flags. */
export interface PerfChoice {
  readonly durationMs: number | null;
  readonly out: string | null;
  readonly minimum: LogLevel;
}

interface PerfFlags {
  readonly duration?: string;
  readonly out?: string;
  readonly level?: string;
}

const secondsSchema = z.coerce
  .number()
  .positive()
  .max(24 * 60 * 60);

/**
 * Reads `--duration`, `--out` (resolved against `cwd`) and `--level`, which
 * defaults to `warn` so that log lines do not drown the samples.
 */
export function parsePerfFlags(flags: PerfFlags, command: string, cwd: string): PerfChoice {
  let durationMs: number | null = null;
  if (flags.duration !== undefined) {
    const seconds = secondsSchema.safeParse(flags.duration);
    if (!seconds.success) {
      throw new UsageError(
        `--duration must be a number of seconds, got ${flags.duration}`,
        command,
      );
    }
    durationMs = seconds.data * 1000;
  }
  return {
    durationMs,
    out: flags.out === undefined ? null : resolve(cwd, flags.out),
    minimum: flags.level === undefined ? 'warn' : parseLevel(flags.level, command),
  };
}
