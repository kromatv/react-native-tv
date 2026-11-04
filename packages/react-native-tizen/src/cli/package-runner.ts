import { existsSync } from 'node:fs';
import { dirname, join } from 'node:path';

const LOCKFILES: readonly (readonly [string, string])[] = [
  ['bun.lock', 'bunx'],
  ['bun.lockb', 'bunx'],
  ['pnpm-lock.yaml', 'pnpm exec'],
  ['yarn.lock', 'yarn'],
  ['package-lock.json', 'npx'],
];

/**
 * How the app's package manager runs a package's command, from the nearest
 * lockfile at or above `dir`: `bunx`, `pnpm exec`, `yarn`, else `npx`. In a Bun
 * workspace `npx rn-tizen` asks the npm registry for a package of that name.
 */
export function packageRunner(dir: string): string {
  const found = LOCKFILES.find(([name]) => existsSync(join(dir, name)));
  if (found !== undefined) return found[1];
  const parent = dirname(dir);
  return parent === dir ? 'npx' : packageRunner(parent);
}
