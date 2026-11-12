import { readFileSync } from 'node:fs';
import { bunPatch } from './bun-patch.ts';
import { addExpoCliHook, hasExpoCliHook } from './expo-cli-hook.ts';
import { expoCliPackage } from './expo-cli-package.ts';
import { inPlacePatch } from './in-place-patch.ts';
import { installRoot, type PackageManager } from './package-manager.ts';
import type { PatchMethod } from './patch-method.ts';
import { pnpmPatch } from './pnpm-patch.ts';
import { runManager, type RunManager } from './run-manager.ts';
import { yarnPatch } from './yarn-patch.ts';

interface Applied<State extends string> {
  readonly state: State;
  readonly version: string;
  readonly manager: PackageManager;
  readonly installDir: string;
  /** The files to commit, relative to `installDir`. */
  readonly changed: readonly string[];
  /** The versions of Expo CLI whose patch, left behind by an upgrade, was removed. */
  readonly removed: readonly string[];
}

/** An Expo CLI that loads this package's commands, and what the run changed to get or keep it so. */
export type ExpoPatchApplied = Applied<'patched'> | Applied<'present'>;

/** What became of the patch: applied, there already, or why it could not be applied. */
export type ExpoPatchOutcome =
  | ExpoPatchApplied
  | { readonly state: 'no-expo' }
  | { readonly state: 'unknown-layout'; readonly version: string }
  | { readonly state: 'no-lockfile' };

const METHODS: Readonly<Record<PackageManager, PatchMethod>> = {
  bun: bunPatch,
  pnpm: pnpmPatch,
  yarn: yarnPatch,
  'yarn-classic': inPlacePatch,
  npm: inPlacePatch,
};

/**
 * Makes the Expo CLI of the app at `projectRoot` load this package's commands,
 * in the way the app's package manager keeps across installs: its patch
 * feature where it has one, and the app's `postinstall` script for npm and
 * Yarn 1. A patch of ours for another version of Expo CLI is removed.
 */
export function patchExpoCli(projectRoot: string, run: RunManager = runManager): ExpoPatchOutcome {
  const expoCli = expoCliPackage(projectRoot);
  if (expoCli === null) return { state: 'no-expo' };
  const { version } = expoCli;
  const source = readFileSync(expoCli.entry, 'utf8');
  const hooked = hasExpoCliHook(source);
  if (!hooked && addExpoCliHook(source) === null) return { state: 'unknown-layout', version };
  const install = installRoot(projectRoot);
  if (install === null) return { state: 'no-lockfile' };

  const method = METHODS[install.manager];
  const target = { projectRoot, install, expoCli, hooked, run };
  const { removed, changed } = method.settle(target);
  const applied = { version, manager: install.manager, installDir: install.dir, removed };
  if (hooked) return { state: 'present', ...applied, changed };
  const written = method.apply(target);
  if (written === null) return { state: 'unknown-layout', version };
  return { state: 'patched', ...applied, changed: [...new Set([...changed, ...written])] };
}
