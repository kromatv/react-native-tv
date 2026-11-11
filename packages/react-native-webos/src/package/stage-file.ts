import { existsSync } from 'node:fs';
import { copyFile, readFile, stat, utimes } from 'node:fs/promises';

async function sameBytes(left: string, right: string): Promise<boolean> {
  const [leftStat, rightStat] = await Promise.all([stat(left), stat(right)]);
  if (leftStat.size !== rightStat.size) return false;
  const [leftBytes, rightBytes] = await Promise.all([readFile(left), readFile(right)]);
  return leftBytes.equals(rightBytes);
}

/**
 * Copies `from` to `to` unless `to` already holds the same bytes, and dates a
 * new copy now. The Tizen .NET SDK copies a file into the package only when it
 * is newer than its last copy there, so a copy that kept an older time, such as
 * a release runtime built before the development one, would never reach it.
 */
export async function stageFile(from: string, to: string): Promise<void> {
  if (existsSync(to) && (await sameBytes(from, to))) return;
  await copyFile(from, to);
  const now = new Date();
  await utimes(to, now, now);
}
