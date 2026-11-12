import { readFileSync, statSync } from 'node:fs';

/** The text of `file`, or null when there is no such file or it is larger than `maxBytes`. */
export function readText(file: string, maxBytes: number): string | null {
  try {
    return statSync(file).size > maxBytes ? null : readFileSync(file, 'utf8');
  } catch {
    return null;
  }
}
