import { isAbsolute, join } from 'node:path';
import { patchEntries, removeOldPatchEntries, type PatchEntries } from './old-patch-entries.ts';
import { entryIn, type PatchMethod } from './patch-method.ts';
import { writeExpoCliHook } from './write-expo-cli-hook.ts';

const EDIT_DIRECTORY = /bun patch --commit '([^'\n]+)'/;
const PATCHED_PACKAGE = /^@expo\/cli@(.+)$/;

const ENTRIES: PatchEntries = {
  member: 'patchedDependencies',
  read: (key, file) => {
    const version = PATCHED_PACKAGE.exec(key)?.[1];
    return version === undefined ? null : { version, file };
  },
};

/**
 * Bun's patch feature: `bun patch` names a directory to edit, and
 * `bun patch --commit` turns the edit into a patch file and an entry under
 * `patchedDependencies` in `package.json`. Bun has no command that removes
 * one, so an entry for a version that is gone is removed from the text.
 */
export const bunPatch: PatchMethod = {
  settle: ({ install, expoCli }) => {
    const removed = removeOldPatchEntries(
      install.dir,
      ENTRIES,
      (entry) => entry.version !== expoCli.version,
    );
    return { removed, changed: removed.length === 0 ? [] : ['package.json'] };
  },
  apply: (target) => {
    const { install, expoCli, run } = target;
    const printed = run('bun', ['patch', `@expo/cli@${expoCli.version}`], install.dir);
    const editDir = EDIT_DIRECTORY.exec(printed)?.[1];
    if (editDir === undefined) throw new Error(`bun patch named no directory to edit:\n${printed}`);
    const copy = isAbsolute(editDir) ? editDir : join(install.dir, editDir);
    if (!writeExpoCliHook(entryIn(copy, target))) return null;
    run('bun', ['patch', '--commit', editDir], install.dir);
    const patches = patchEntries(install.dir, ENTRIES).filter(
      (entry) => entry.version === expoCli.version,
    );
    return [...patches.map((entry) => entry.file), 'package.json', install.lockfile];
  },
};
