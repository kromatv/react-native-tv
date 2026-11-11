import { isAbsolute, relative } from 'node:path';

/** A step's duration the way a person reads it: `0.4s`, `12s`, `2m 05s`. */
export function formatDuration(milliseconds: number): string {
  const seconds = milliseconds / 1000;
  if (seconds < 10) return `${seconds.toFixed(1)}s`;
  if (seconds < 60) return `${Math.round(seconds)}s`;
  const minutes = Math.floor(seconds / 60);
  const rest = Math.round(seconds - minutes * 60);
  return `${minutes}m ${String(rest).padStart(2, '0')}s`;
}

const UNITS = ['B', 'KB', 'MB', 'GB'] as const;

/** A file size the way a person reads it: `912 B`, `4.1 MB`. */
export function formatSize(bytes: number): string {
  let size = bytes;
  let unit = 0;
  while (size >= 1000 && unit < UNITS.length - 1) {
    size /= 1000;
    unit += 1;
  }
  return unit === 0 ? `${size} B` : `${size.toFixed(1)} ${UNITS[unit]}`;
}

/** `path` as typed from `cwd`: relative inside it, absolute outside it. */
export function formatPath(path: string, cwd: string): string {
  const fromCwd = relative(cwd, path);
  return fromCwd.startsWith('..') || isAbsolute(fromCwd) ? path : fromCwd;
}
