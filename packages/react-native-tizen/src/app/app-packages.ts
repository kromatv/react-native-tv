import { readFileSync } from 'node:fs';
import { createRequire } from 'node:module';
import { dirname, join } from 'node:path';
import { z } from 'zod';
import { FixableError } from '../errors.ts';

/** An npm package as the app resolves it. */
export interface ResolvedPackage {
  readonly name: string;
  readonly version: string;
  readonly dir: string;
}

/** The packages the native runtime is compiled against, each the app's own copy. */
export interface AppPackages {
  readonly reactNative: ResolvedPackage;
  readonly expo: ResolvedPackage;
  readonly expoModulesCore: ResolvedPackage;
  readonly expoModulesJsi: ResolvedPackage;
}

const packageJsonSchema = z.object({ name: z.string(), version: z.string() });

/** Resolves `name` the way code in `fromDir` would, or null when it is not installed there. */
export function findPackage(fromDir: string, name: string): ResolvedPackage | null {
  let manifest: string;
  try {
    manifest = createRequire(join(fromDir, 'package.json')).resolve(`${name}/package.json`);
  } catch {
    return null;
  }
  const parsed = packageJsonSchema.parse(JSON.parse(readFileSync(manifest, 'utf8')));
  return { name: parsed.name, version: parsed.version, dir: dirname(manifest) };
}

function requirePackage(fromDir: string, name: string, owner: string): ResolvedPackage {
  const found = findPackage(fromDir, name);
  if (found !== null) return found;
  throw new FixableError(
    `${name} is not installed for ${owner}`,
    "Install the app's dependencies (npm install, or your package manager's equivalent), then try again.",
  );
}

/** The app's react-native, expo and the Expo native packages behind it. */
export function resolveAppPackages(projectRoot: string): AppPackages {
  const reactNative = requirePackage(projectRoot, 'react-native', 'this app');
  const expo = requirePackage(projectRoot, 'expo', 'this app');
  const expoModulesCore =
    findPackage(projectRoot, 'expo-modules-core') ??
    requirePackage(expo.dir, 'expo-modules-core', 'expo');
  const expoModulesJsi = requirePackage(
    expoModulesCore.dir,
    'expo-modules-jsi',
    'expo-modules-core',
  );
  return { reactNative, expo, expoModulesCore, expoModulesJsi };
}
