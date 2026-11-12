import { z } from 'zod';
import type { Paint } from '../paint.ts';

/** The levels the native host logs at, quietest first. */
export const LOG_LEVELS = ['debug', 'info', 'warn', 'error'] as const;

/** One of {@link LOG_LEVELS}. */
export type LogLevel = (typeof LOG_LEVELS)[number];

/** A log line a TV streamed. */
export interface LogEntry {
  readonly level: LogLevel;
  readonly message: string;
}

const logEntrySchema = z.object({
  level: z.enum(LOG_LEVELS).catch('info'),
  message: z.string(),
});

const LEVEL_STYLE = {
  debug: 'gray',
  info: 'cyan',
  warn: 'yellow',
  error: 'red',
} as const satisfies Record<LogLevel, Parameters<Paint>[0]>;

const CONTROL_CHARACTERS = /[\u0000-\u0008\u000B-\u001F\u007F-\u009F]/g;

const LABEL_WIDTH = 5;
const TIME_WIDTH = 8;

function parseJson(line: string): unknown {
  try {
    return JSON.parse(line);
  } catch {
    return null;
  }
}

/**
 * `text` without its C0 and C1 control characters, line feed and tab kept, so
 * what a TV sends cannot move the cursor, recolor or retitle the terminal.
 */
export function printable(text: string): string {
  return text.replace(CONTROL_CHARACTERS, '');
}

/**
 * Reads a JSON log line (`{"level":"info","message":"..."}`). Anything else,
 * such as a crash dump written raw, is kept as an info line. Either way the
 * message is {@link printable}.
 */
export function parseLogLine(line: string): LogEntry {
  const parsed = logEntrySchema.safeParse(parseJson(line));
  const entry = parsed.success ? parsed.data : { level: 'info' as const, message: line };
  return { level: entry.level, message: printable(entry.message) };
}

/** True when `level` is at least as loud as `minimum`. */
export function isAtLeast(level: LogLevel, minimum: LogLevel): boolean {
  return LOG_LEVELS.indexOf(level) >= LOG_LEVELS.indexOf(minimum);
}

function clock(time: Date): string {
  return time.toTimeString().slice(0, TIME_WIDTH);
}

function messageStyle(level: LogLevel): Parameters<Paint>[0] | null {
  if (level === 'debug') return 'gray';
  if (level === 'error') return 'red';
  return null;
}

/**
 * Formats an entry as `HH:MM:SS level message`, the level colored, a multi-line
 * message indented under its first line.
 */
export function formatLogEntry(entry: LogEntry, time: Date, paint: Paint): string {
  const indent = ' '.repeat(TIME_WIDTH + LABEL_WIDTH + 2);
  const body = entry.message.replace(/\r?\n/g, `\n${indent}`);
  const style = messageStyle(entry.level);
  return [
    paint('gray', clock(time)),
    paint(LEVEL_STYLE[entry.level], entry.level.padEnd(LABEL_WIDTH)),
    style === null ? body : paint(style, body),
  ].join(' ');
}
