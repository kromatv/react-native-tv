import { join, relative } from 'node:path';
import type { PatchMethod } from './patch-method.ts';
import { addPostinstallScript } from './postinstall-script.ts';
import { writeExpoCliHook } from './write-expo-cli-hook.ts';

/**
 * For npm and Yarn 1, which have no patch feature: the hook goes into the
 * installed file, and the app's `postinstall` script runs `rn-tv expo-patch`
 * so every install puts it back.
 */
export const inPlacePatch: PatchMethod = {
  settle: ({ projectRoot, install }) => {
    const manifest = join(projectRoot, 'package.json');
    const changed = addPostinstallScript(manifest) ? [relative(install.dir, manifest)] : [];
    return { removed: [], changed };
  },
  apply: ({ expoCli }) => (writeExpoCliHook(expoCli.entry) ? [] : null),
};
