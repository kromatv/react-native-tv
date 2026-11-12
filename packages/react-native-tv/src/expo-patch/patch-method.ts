import { join, relative } from 'node:path';
import type { ExpoCliPackage } from './expo-cli-package.ts';
import type { InstallRoot } from './package-manager.ts';
import type { RunManager } from './run-manager.ts';

/** The Expo CLI to patch, the app it belongs to and the package manager that installed it. */
export interface PatchTarget {
  readonly projectRoot: string;
  readonly install: InstallRoot;
  readonly expoCli: ExpoCliPackage;
  /** Whether the installed Expo CLI loads this package's commands already. */
  readonly hooked: boolean;
  readonly run: RunManager;
}

/** What a run settled before patching: the files it changed, relative to the install directory. */
export interface Settled {
  /** The versions of Expo CLI whose patch, left behind by an upgrade, was removed. */
  readonly removed: readonly string[];
  readonly changed: readonly string[];
}

/** How one package manager keeps the hook in an app's Expo CLI across installs. */
export interface PatchMethod {
  /** Runs every time: removes what an Expo upgrade left behind and keeps the method's own set-up. */
  settle(target: PatchTarget): Settled;
  /**
   * Adds the hook, and gives the files to commit, relative to the install
   * directory, or null when the copy to patch is laid out in an unknown way.
   */
  apply(target: PatchTarget): readonly string[] | null;
}

/** The entry of the Expo CLI of `target`, inside a copy of the package at `copy`. */
export function entryIn(copy: string, { expoCli }: PatchTarget): string {
  return join(copy, relative(expoCli.dir, expoCli.entry));
}
