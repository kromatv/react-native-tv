import { existsSync, mkdirSync, readdirSync, readFileSync, statSync, writeFileSync } from 'node:fs';
import { join } from 'node:path';
import { FixableError } from '../errors.ts';

const IGNORE_FILE = '.gitignore';
const IGNORE_ALL = '*\n';

function isGenerated(dir: string): boolean {
  if (!statSync(dir).isDirectory()) return false;
  if (readdirSync(dir).length === 0) return true;
  const ignore = join(dir, IGNORE_FILE);
  return existsSync(ignore) && readFileSync(ignore, 'utf8').trim() === '*';
}

/**
 * Makes `dir` the app's generated `webos/` directory, which ignores itself so
 * it never reaches git. Refuses a `webos/` the app keeps for itself: one that
 * holds files and does not ignore itself.
 */
export function claimWebOSDir(dir: string): void {
  if (existsSync(dir) && !isGenerated(dir)) {
    throw new FixableError(
      `${dir} is the app's own, not a directory rn-webos generated`,
      'rn-webos writes its webOS project to webos/ and replaces it on every build: move what is there out of the app, then run the command again.',
    );
  }
  mkdirSync(dir, { recursive: true });
  writeFileSync(join(dir, IGNORE_FILE), IGNORE_ALL);
}
