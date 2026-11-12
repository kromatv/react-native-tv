import { mkdtempSync, realpathSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join, relative } from 'node:path';
import { z } from 'zod';
import { isOurPatch } from './our-patch.ts';
import { entryIn, type PatchMethod, type PatchTarget } from './patch-method.ts';
import { readText } from './read-text.ts';
import { writeExpoCliHook } from './write-expo-cli-hook.ts';

const PATCHED_PACKAGE = /^@expo\/cli@(.+)$/;
const WORKSPACE_FILE = 'pnpm-workspace.yaml';
const MAX_WORKSPACE_FILE_BYTES = 4 * 1024 * 1024;

const patchesSchema = z.record(z.string(), z.string());

function patchFiles({ install, run }: PatchTarget): Map<string, string> {
  const printed = run('pnpm', ['config', 'get', 'patchedDependencies', '--json'], install.dir);
  const files = new Map<string, string>();
  try {
    for (const [key, file] of Object.entries(patchesSchema.parse(JSON.parse(printed)))) {
      const version = PATCHED_PACKAGE.exec(key)?.[1];
      if (version !== undefined) files.set(version, file);
    }
  } catch {
    // pnpm prints nothing or `null` when the app has no patch.
  }
  return files;
}

// pnpm prints the real path of a patch file, which a symlink on the way to the
// app makes differ from the path the app was reached by.
function fromInstallDir(installDir: string, file: string): string {
  const direct = relative(installDir, file);
  return direct.startsWith('..') ? relative(realpathSync(installDir), file) : direct;
}

function settingsFile({ install }: PatchTarget, version: string): string {
  const workspace = readText(join(install.dir, WORKSPACE_FILE), MAX_WORKSPACE_FILE_BYTES);
  return workspace?.includes(`@expo/cli@${version}`) === true ? WORKSPACE_FILE : 'package.json';
}

/**
 * pnpm's patch feature: `pnpm patch` extracts the package to a directory to
 * edit, `pnpm patch-commit` turns the edit into a patch file and an entry
 * under `patchedDependencies`, and `pnpm patch-remove` removes both. pnpm
 * writes its settings itself, so their formatting is pnpm's.
 */
export const pnpmPatch: PatchMethod = {
  settle: (target) => {
    const { install, expoCli, run } = target;
    const old = [...patchFiles(target)].filter(
      ([version, file]) => version !== expoCli.version && isOurPatch(file),
    );
    const [first] = old;
    if (first === undefined) return { removed: [], changed: [] };
    const settings = settingsFile(target, first[0]);
    const packages = old.map(([version]) => `@expo/cli@${version}`);
    run('pnpm', ['patch-remove', ...packages], install.dir);
    return { removed: old.map(([version]) => version), changed: [settings, install.lockfile] };
  },
  apply: (target) => {
    const { install, expoCli, run } = target;
    // pnpm refuses its own edit directory, under node_modules, when an earlier
    // `pnpm patch` left it there: a new one each time is never in the way.
    const scratch = mkdtempSync(join(tmpdir(), 'rn-tv-expo-patch-'));
    try {
      const copy = join(scratch, 'cli');
      run('pnpm', ['patch', '--edit-dir', copy, `@expo/cli@${expoCli.version}`], install.dir);
      if (!writeExpoCliHook(entryIn(copy, target))) return null;
      run('pnpm', ['patch-commit', copy], install.dir);
    } finally {
      rmSync(scratch, { recursive: true, force: true });
    }
    const patch = patchFiles(target).get(expoCli.version);
    return [
      ...(patch === undefined ? [] : [fromInstallDir(install.dir, patch)]),
      settingsFile(target, expoCli.version),
      install.lockfile,
    ];
  },
};
