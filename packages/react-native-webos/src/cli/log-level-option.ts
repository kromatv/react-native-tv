import { LOG_LEVELS, type LogLevel } from '../logs/log-line.ts';
import { UsageError, type OptionSpecs } from './command.ts';

/** The option that hides quieter log levels. */
export const LEVEL_OPTIONS = {
  level: {
    type: 'string',
    placeholder: '<level>',
    description: 'The quietest level to show: debug, info, warn or error (default: info)',
  },
} as const satisfies OptionSpecs;

function isLogLevel(value: string): value is LogLevel {
  return (LOG_LEVELS as readonly string[]).includes(value);
}

/** Reads `--level`, info when absent. */
export function parseLevel(value: string | undefined, command: string): LogLevel {
  if (value === undefined) return 'info';
  if (isLogLevel(value)) return value;
  throw new UsageError(`--level must be one of ${LOG_LEVELS.join(', ')}, got ${value}`, command);
}

/** Resolves on Ctrl+C, so a command that streams can clean up before exiting. */
export function untilInterrupted(): Promise<void> {
  return new Promise((resolve) => {
    process.once('SIGINT', () => resolve());
  });
}
