import { join } from 'node:path';
import { z } from 'zod';
import { parseVersion } from '../doctor/check-result.ts';
import { runSync } from '../exec.ts';
import { objectAt, readJsonObject, writeJsonObject, type JsonObject } from './json-file.ts';
import { unlockUpstreamReactNative } from './npm-lock.ts';
import type { OverrideSetup } from './override-setup.ts';
import { findPackageManager, type PackageManager } from './package-manager.ts';
import { setPnpmWorkspaceOverride } from './pnpm-workspace-override.ts';
import { isReactNativeTvosSpec, REACT_NATIVE_TVOS_SPEC } from './react-native-tvos.ts';

/** What `setUpReactNative` changed so that the app installs one react-native-tvos. */
export interface ReactNativeSetup {
  readonly manager: PackageManager;
  readonly alias: {
    readonly kind: 'set' | 'present';
    readonly spec: string;
    readonly file: string;
  };
  readonly override: OverrideSetup;
  readonly unlocked: boolean;
}

// pnpm 12 reads overrides from pnpm-workspace.yaml only; pnpm 9 refuses that file without `packages`.
const PNPM_WORKSPACE_OVERRIDES_MAJOR = 11;
const PNPM_VERSION_TIMEOUT_MS = 30_000;
const DIRECT_REFERENCE = '$react-native';
const specSchema = z.string();

/** The major version of the pnpm that installs in `root`. */
export function pnpmMajor(root: string): number {
  const output = runSync(['pnpm', '--version'], { cwd: root, timeoutMs: PNPM_VERSION_TIMEOUT_MS });
  const version = parseVersion(output);
  return version === null ? PNPM_WORKSPACE_OVERRIDES_MAJOR : Number(version.split('.')[0]);
}

function setAlias(appRoot: string): ReactNativeSetup['alias'] {
  const file = join(appRoot, 'package.json');
  const manifest = readJsonObject(file);
  const dependencies = objectAt(manifest, 'dependencies');
  const current = specSchema.safeParse(dependencies['react-native']);
  if (current.success && isReactNativeTvosSpec(current.data)) {
    return { kind: 'present', spec: current.data, file };
  }
  const next = { ...dependencies, 'react-native': REACT_NATIVE_TVOS_SPEC };
  writeJsonObject(file, { ...manifest, dependencies: next });
  return { kind: 'set', spec: REACT_NATIVE_TVOS_SPEC, file };
}

function withEntry(holder: JsonObject, keys: readonly string[], value: string): JsonObject {
  const [key, ...rest] = keys;
  if (key === undefined) return { ...holder, 'react-native': value };
  return { ...holder, [key]: withEntry(objectAt(holder, key), rest, value) };
}

function entryAt(holder: JsonObject, keys: readonly string[]): unknown {
  const [key, ...rest] = keys;
  return key === undefined ? holder['react-native'] : entryAt(objectAt(holder, key), rest);
}

function setInPackageJson(root: string, keys: readonly string[], value: string): OverrideSetup {
  const file = join(root, 'package.json');
  const manifest = readJsonObject(file);
  if (entryAt(manifest, keys) === value) return { kind: 'present', file };
  writeJsonObject(file, withEntry(manifest, keys, value));
  return { kind: 'set', file };
}

/**
 * npm, Bun and pnpm take `$react-native`, the root's own spec, so the override
 * follows an upgrade; Yarn takes only a spec, as does a root that is not the app.
 */
function overrideValue(manager: PackageManager, spec: string): string {
  if (manager.name === 'yarn') return spec;
  const rootDependencies = objectAt(
    readJsonObject(join(manager.root, 'package.json')),
    'dependencies',
  );
  return rootDependencies['react-native'] === undefined ? spec : DIRECT_REFERENCE;
}

function setOverride(
  manager: PackageManager,
  spec: string,
  majorOfPnpm: (root: string) => number,
): OverrideSetup {
  const value = overrideValue(manager, spec);
  switch (manager.name) {
    case 'npm':
    case 'bun':
      return setInPackageJson(manager.root, ['overrides'], value);
    case 'yarn':
      return setInPackageJson(manager.root, ['resolutions'], value);
    case 'pnpm':
      return majorOfPnpm(manager.root) < PNPM_WORKSPACE_OVERRIDES_MAJOR
        ? setInPackageJson(manager.root, ['pnpm', 'overrides'], value)
        : setPnpmWorkspaceOverride(manager.root, value);
  }
}

/**
 * Points the app's `react-native` at react-native-tvos and overrides every
 * other `react-native` in the tree with it, where the app's package manager
 * reads overrides. Changes nothing that is already so.
 */
export function setUpReactNative(
  appRoot: string,
  majorOfPnpm: (root: string) => number = pnpmMajor,
): ReactNativeSetup {
  const manager = findPackageManager(appRoot);
  const alias = setAlias(appRoot);
  const override = setOverride(manager, alias.spec, majorOfPnpm);
  const unlocked = manager.name === 'npm' && unlockUpstreamReactNative(manager.root);
  return { manager, alias, override, unlocked };
}
