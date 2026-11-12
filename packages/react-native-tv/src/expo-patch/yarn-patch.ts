import { join } from 'node:path';
import { z } from 'zod';
import { readManifest } from './app-manifest.ts';
import { removeOldPatchEntries, type PatchEntries } from './old-patch-entries.ts';
import { entryIn, type PatchMethod } from './patch-method.ts';
import { writeExpoCliHook } from './write-expo-cli-hook.ts';

const EXPO_CLI_PATCH = 'patch:@expo/cli@';
const PATCH_FILE = /#~\/([^#:]+)$/;
const PATCH_RESOLUTION = /^patch:@expo\/cli@npm%3A([^#]+)#~\/([^#:]+)$/;

const ENTRIES: PatchEntries = {
  member: 'resolutions',
  read: (_descriptor, resolution) => {
    const [, version, file] = PATCH_RESOLUTION.exec(resolution) ?? [];
    return version === undefined || file === undefined ? null : { version, file };
  },
};

const preparedSchema = z.object({
  locator: z.string().max(1024),
  path: z.string().min(1).max(4096),
});

function resolutionsIn(installDir: string): string[] {
  return Object.values(readManifest(join(installDir, 'package.json'))?.resolutions ?? {});
}

function prepared(printed: string): z.infer<typeof preparedSchema> {
  try {
    const trimmed = printed.trim();
    return preparedSchema.parse(JSON.parse(trimmed.slice(trimmed.lastIndexOf('\n') + 1)));
  } catch {
    throw new Error(`yarn patch named no directory to edit:\n${printed}`);
  }
}

/**
 * The patch feature of Yarn 2 and later: `yarn patch` extracts the package to
 * a directory to edit, `yarn patch-commit -s` turns the edit into a patch file
 * and a `resolutions` entry in `package.json`, and an install applies it.
 * Yarn keys the entry by the range `expo` asks for, so one for the installed
 * version can be out of use too: without the hook, every entry of ours is old.
 */
export const yarnPatch: PatchMethod = {
  settle: ({ install, expoCli, hooked }) => {
    const removed = removeOldPatchEntries(
      install.dir,
      ENTRIES,
      (entry) => !hooked || entry.version !== expoCli.version,
    );
    return {
      removed: removed.filter((version) => version !== expoCli.version),
      changed: removed.length === 0 ? [] : ['package.json'],
    };
  },
  apply: (target) => {
    const { install, expoCli, run } = target;
    const before = resolutionsIn(install.dir);
    // Asked by name with --update, Yarn starts from the patch the app has for the
    // package and refuses a package with none; asked by locator, it starts from
    // the package as published, patch or not.
    const { locator, path } = prepared(
      before.some((resolution) => resolution.startsWith(EXPO_CLI_PATCH))
        ? run('yarn', ['patch', '--update', '--json', '@expo/cli'], install.dir)
        : run('yarn', ['patch', '--json', `@expo/cli@npm:${expoCli.version}`], install.dir),
    );
    if (!locator.endsWith(`:${expoCli.version}`)) {
      throw new Error(
        `yarn patch prepared ${locator}, and this app runs @expo/cli ${expoCli.version}.`,
      );
    }
    if (!writeExpoCliHook(entryIn(path, target))) return null;
    run('yarn', ['patch-commit', '-s', path], install.dir);
    run('yarn', ['install'], install.dir);
    const patches = resolutionsIn(install.dir)
      .filter((resolution) => !before.includes(resolution))
      .flatMap((resolution) => PATCH_FILE.exec(resolution)?.[1] ?? []);
    return [...new Set(patches), 'package.json', install.lockfile];
  },
};
