import { existsSync } from 'node:fs';
import { join } from 'node:path';
import { findPackage, type ResolvedPackage } from '../app/app-packages.ts';
import type { Task } from '../task.ts';
import { ensureHermesHostCompilers } from '../runtime/hermes-host.ts';

const PREBUILT_DIR: Partial<Record<NodeJS.Platform, string>> = {
  darwin: 'osx-bin',
  linux: 'linux64-bin',
};

function prebuiltHermesc(reactNative: ResolvedPackage): string | null {
  const dir = PREBUILT_DIR[process.platform];
  const compiler = findPackage(reactNative.dir, 'hermes-compiler');
  if (dir === undefined || compiler === null) return null;
  const hermesc = join(compiler.dir, 'hermesc', dir, 'hermesc');
  return existsSync(hermesc) ? hermesc : null;
}

/**
 * The bytecode compiler for a bundle: the one react-native ships, which matches
 * its Hermes, or the host build of the pinned Hermes when there is none.
 */
export async function locateHermesc(task: Task, reactNative: ResolvedPackage): Promise<string> {
  return prebuiltHermesc(reactNative) ?? (await ensureHermesHostCompilers(task)).hermesc;
}
