import { existsSync } from 'node:fs';
import { dirname, join } from 'node:path';

/** A package manager `rn-tizen` knows how to set an app up with. */
export type PackageManagerName = 'bun' | 'pnpm' | 'yarn' | 'npm';

/** The app's package manager and the root it installs from, where its lockfile is. */
export interface PackageManager {
  readonly name: PackageManagerName;
  readonly root: string;
}

const LOCKFILES: readonly (readonly [string, PackageManagerName])[] = [
  ['bun.lock', 'bun'],
  ['bun.lockb', 'bun'],
  ['pnpm-lock.yaml', 'pnpm'],
  ['yarn.lock', 'yarn'],
  ['package-lock.json', 'npm'],
];

function lockedManager(dir: string): PackageManager | null {
  const found = LOCKFILES.find(([name]) => existsSync(join(dir, name)));
  if (found !== undefined && existsSync(join(dir, 'package.json'))) {
    return { name: found[1], root: dir };
  }
  const parent = dirname(dir);
  return parent === dir ? null : lockedManager(parent);
}

/**
 * The package manager of the nearest lockfile at or above `dir`, the root of a
 * workspace for an app inside one. Without a lockfile, npm in `dir`.
 */
export function findPackageManager(dir: string): PackageManager {
  return lockedManager(dir) ?? { name: 'npm', root: dir };
}

const RUNNERS: Readonly<Record<PackageManagerName, string>> = {
  bun: 'bunx',
  pnpm: 'pnpm exec',
  yarn: 'yarn',
  npm: 'npx',
};

/**
 * How the app's package manager runs a package's command: `bunx`, `pnpm exec`,
 * `yarn`, else `npx`. In a Bun workspace `npx rn-tizen` asks the npm registry
 * for a package of that name.
 */
export function packageRunner(dir: string): string {
  return RUNNERS[findPackageManager(dir).name];
}
