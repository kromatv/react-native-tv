import { existsSync, readFileSync, realpathSync } from 'node:fs';
import { basename, dirname, join, relative } from 'node:path';
import { z } from 'zod';
import { FixableError } from '../errors.ts';
import { packageRunner } from './package-manager.ts';

/** One `react-native` the app's packages load, and who loads it. */
export interface ReactNativeCopy {
  readonly name: string;
  readonly version: string;
  readonly dir: string;
  readonly dependents: readonly string[];
}

const dependencySchema = z.record(z.string(), z.string()).optional();
const manifestSchema = z.looseObject({
  name: z.string().optional(),
  version: z.string().optional(),
  dependencies: dependencySchema,
  devDependencies: dependencySchema,
  optionalDependencies: dependencySchema,
  peerDependencies: dependencySchema,
});
type Manifest = z.infer<typeof manifestSchema>;

const MAX_MANIFEST_BYTES = 1024 * 1024;
const MAX_PACKAGES = 50_000;

function readManifest(dir: string): Manifest | null {
  try {
    const text = readFileSync(join(dir, 'package.json'), 'utf8');
    if (text.length > MAX_MANIFEST_BYTES) return null;
    const parsed = manifestSchema.safeParse(JSON.parse(text));
    return parsed.success ? parsed.data : null;
  } catch {
    return null;
  }
}

function dependencyNames(manifest: Manifest, isApp: boolean): string[] {
  const groups = [
    manifest.dependencies,
    manifest.optionalDependencies,
    manifest.peerDependencies,
    isApp ? manifest.devDependencies : undefined,
  ];
  return [...new Set(groups.flatMap((group) => Object.keys(group ?? {})))];
}

/** Where code in `fromDir` finds `name`: the nearest `node_modules/<name>` above it, as Node and Metro look. */
function locate(fromDir: string, name: string): string | null {
  for (let dir = fromDir; ; dir = dirname(dir)) {
    const candidate = join(dir, 'node_modules', name);
    if (basename(dir) !== 'node_modules' && existsSync(join(candidate, 'package.json'))) {
      return realpathSync(candidate);
    }
    if (dirname(dir) === dir) return null;
  }
}

/**
 * Every `react-native` the app's packages resolve, following each package's
 * dependencies, optional dependencies and peers from where it is installed.
 */
export function reactNativeCopies(appRoot: string): ReactNativeCopy[] {
  const root = realpathSync(appRoot);
  const dependents = new Map<string, string[]>();
  const seen = new Set([root]);
  const queue = [root];
  for (let dir = queue.pop(); dir !== undefined && seen.size <= MAX_PACKAGES; dir = queue.pop()) {
    const manifest = readManifest(dir);
    if (manifest === null) continue;
    for (const name of dependencyNames(manifest, dir === root)) {
      const found = locate(dir, name);
      if (found === null) continue;
      if (name === 'react-native') {
        dependents.set(found, [...(dependents.get(found) ?? []), manifest.name ?? 'the app']);
      }
      if (!seen.has(found)) {
        seen.add(found);
        queue.push(found);
      }
    }
  }
  return [...dependents].map(([dir, users]) => {
    const manifest = readManifest(dir);
    return {
      name: manifest?.name ?? 'react-native',
      version: manifest?.version ?? 'unknown',
      dir,
      dependents: [...users].sort((a, b) => a.localeCompare(b)),
    };
  });
}

function describeCopies(appRoot: string, copies: readonly ReactNativeCopy[]): string {
  const root = realpathSync(appRoot);
  return copies
    .map((copy) => {
      const users = copy.dependents.slice(0, 3).join(', ');
      const more = copy.dependents.length > 3 ? ` and ${copy.dependents.length - 3} more` : '';
      return `${copy.name} ${copy.version} at ${relative(root, copy.dir)}, for ${users}${more}`;
    })
    .join('; ');
}

/** The error for an app that loads more than one `react-native`, with `hint` as the fix. */
export function duplicateReactNativeError(
  appRoot: string,
  copies: readonly ReactNativeCopy[],
  hint: string,
): FixableError {
  return new FixableError(
    `The app loads ${copies.length} copies of react-native: ${describeCopies(appRoot, copies)}`,
    hint,
  );
}

/** Why the app cannot run on Tizen as installed, when it loads more than one `react-native`. */
export function singleReactNativeIssue(appRoot: string): FixableError | null {
  const copies = reactNativeCopies(appRoot);
  if (copies.length < 2) return null;
  return duplicateReactNativeError(
    appRoot,
    copies,
    `Run ${packageRunner(appRoot)} rn-tizen init: it points every package at the app's react-native and reinstalls.`,
  );
}
