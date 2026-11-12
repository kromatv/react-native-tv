import { closeSync, existsSync, openSync, readSync } from 'node:fs';
import { dirname, join } from 'node:path';

/** `yarn` is Yarn 2 and later; Yarn 1 is `yarn-classic`, because it has no patch feature. */
export type PackageManager = 'bun' | 'pnpm' | 'yarn' | 'yarn-classic' | 'npm';

/** The directory that holds an app's lockfile, and the package manager that wrote it. */
export interface InstallRoot {
  readonly manager: PackageManager;
  readonly dir: string;
  readonly lockfile: string;
}

const LOCKFILES: readonly (readonly [string, PackageManager])[] = [
  ['bun.lock', 'bun'],
  ['bun.lockb', 'bun'],
  ['pnpm-lock.yaml', 'pnpm'],
  ['yarn.lock', 'yarn'],
  ['package-lock.json', 'npm'],
];

const YARN_CLASSIC_HEADER = '# yarn lockfile v1';
const HEADER_BYTES = 256;

function startOf(file: string): string {
  const buffer = Buffer.alloc(HEADER_BYTES);
  const descriptor = openSync(file, 'r');
  try {
    return buffer.toString('utf8', 0, readSync(descriptor, buffer, 0, HEADER_BYTES, 0));
  } finally {
    closeSync(descriptor);
  }
}

// Yarn 1 starts its lockfile with this header and Yarn 2 and later never write it, while
// `.yarnrc.yml` and the `packageManager` field can both be absent from either.
function yarnOf(lockfile: string): PackageManager {
  return startOf(lockfile).includes(YARN_CLASSIC_HEADER) ? 'yarn-classic' : 'yarn';
}

/** The nearest lockfile at or above `projectRoot`, which in a workspace is the workspace's. */
export function installRoot(projectRoot: string): InstallRoot | null {
  let dir = projectRoot;
  for (;;) {
    const found = LOCKFILES.find(([lockfile]) => existsSync(join(dir, lockfile)));
    if (found !== undefined) {
      const [lockfile, manager] = found;
      return { manager: manager === 'yarn' ? yarnOf(join(dir, lockfile)) : manager, dir, lockfile };
    }
    const parent = dirname(dir);
    if (parent === dir) return null;
    dir = parent;
  }
}
