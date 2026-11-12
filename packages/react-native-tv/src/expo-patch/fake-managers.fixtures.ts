import { cpSync, mkdtempSync, readFileSync, realpathSync, rmSync } from 'node:fs';
import { join, resolve } from 'node:path';
import type { TestApp } from '../app.fixtures.ts';
import { EXPO_CLI_ENTRY_FILE } from './expo-cli.fixtures.ts';
import type { ManagerCommand, RunManager } from './run-manager.ts';

export type ManagerCall = readonly [ManagerCommand, ...string[]];

const INSTALLED = 'node_modules/@expo/cli';
const ENTRY = 'build/src/index.js';

export const BUN_PRINTED =
  "\n  node_modules/@expo/cli\n\n  bun patch --commit 'node_modules/@expo/cli'\n";

function manifestOf(app: TestApp): Record<string, unknown> {
  const manifest: Record<string, unknown> = JSON.parse(
    readFileSync(join(app.root, 'package.json'), 'utf8'),
  );
  return manifest;
}

function installedVersion(app: TestApp): string {
  const text = readFileSync(join(app.root, INSTALLED, 'package.json'), 'utf8');
  return /"version":"([^"]+)"/.exec(text)?.[1] ?? '';
}

function writeManifest(app: TestApp, manifest: Record<string, unknown>): void {
  app.write('package.json', `${JSON.stringify(manifest, null, 2)}\n`);
}

function install(app: TestApp, copy: string): void {
  app.write(EXPO_CLI_ENTRY_FILE, readFileSync(join(copy, ENTRY), 'utf8'));
}

/**
 * Bun as 1.4.2 was seen to patch: it names the installed package as the
 * directory to edit, and its commit writes the patch file and the
 * `patchedDependencies` entry, rewriting `package.json`.
 */
export function fakeBun(app: TestApp, calls: ManagerCall[], printed = BUN_PRINTED): RunManager {
  return (command, args, cwd) => {
    calls.push([command, ...args]);
    if (args[1] !== '--commit') return printed;
    const version = installedVersion(app);
    const file = `patches/@expo%2Fcli@${version}.patch`;
    app.write(file, readFileSync(resolve(cwd, args[2] ?? '', ENTRY), 'utf8'));
    const manifest = manifestOf(app);
    writeManifest(app, {
      ...manifest,
      patchedDependencies: {
        ...Object(manifest.patchedDependencies),
        [`@expo/cli@${version}`]: file,
      },
    });
    return '';
  };
}

/**
 * pnpm as 11.2.2 and 12.18.0 were seen to patch: `patches` is what its
 * settings hold, by package, as paths from the app. A commit and a removal
 * write `pnpm-workspace.yaml` and install.
 */
export function fakePnpm(
  app: TestApp,
  calls: ManagerCall[],
  patches: Map<string, string> = new Map(),
): RunManager {
  const writeSettings = () =>
    app.write(
      'pnpm-workspace.yaml',
      [...patches].reduce(
        (yaml, [key, file]) => `${yaml}  '${key}': ${file}\n`,
        patches.size === 0 ? '' : 'patchedDependencies:\n',
      ),
    );
  return (command, args) => {
    calls.push([command, ...args]);
    const [verb, ...rest] = args;
    if (verb === 'config') {
      if (patches.size === 0) return 'undefined\n';
      const absolute = [...patches].map(([key, file]) => [key, join(realpathSync(app.root), file)]);
      return `${JSON.stringify(Object.fromEntries(absolute), null, 2)}\n`;
    }
    if (verb === 'patch') {
      cpSync(join(app.root, INSTALLED), rest[1] ?? '', { recursive: true });
      return `Patch: You can now edit the package at:\n\n  ${rest[1]}\n`;
    }
    if (verb === 'patch-commit') {
      const version = installedVersion(app);
      const file = `patches/@expo__cli@${version}.patch`;
      app.write(file, readFileSync(join(rest[0] ?? '', ENTRY), 'utf8'));
      patches.set(`@expo/cli@${version}`, file);
      install(app, rest[0] ?? '');
    }
    if (verb === 'patch-remove') {
      for (const key of rest) {
        rmSync(join(app.root, patches.get(key) ?? ''));
        patches.delete(key);
      }
    }
    writeSettings();
    return '';
  };
}

/**
 * Yarn as 4.18.1 was seen to patch: it extracts the package to a directory of
 * its own, its commit writes the patch file and a `resolutions` entry keyed
 * by `range`, and only an install applies it.
 */
export function fakeYarn(app: TestApp, calls: ManagerCall[], range = '^58.1.19'): RunManager {
  let committed = '';
  return (command, args) => {
    calls.push([command, ...args]);
    const version = installedVersion(app);
    if (args[0] === 'patch') {
      const copy = join(mkdtempSync(join(app.root, 'xfs-')), 'user');
      cpSync(join(app.root, INSTALLED), copy, { recursive: true });
      const locator = `@expo/cli@npm:${version}`;
      return `${JSON.stringify({ locator, path: copy })}\n`;
    }
    if (args[0] === 'patch-commit') {
      committed = args[2] ?? '';
      const file = `.yarn/patches/@expo-cli-npm-${version}-b9cf9fdd52.patch`;
      app.write(file, readFileSync(join(committed, ENTRY), 'utf8'));
      const manifest = manifestOf(app);
      writeManifest(app, {
        ...manifest,
        resolutions: {
          ...Object(manifest.resolutions),
          [`@expo/cli@npm:${range}`]: `patch:@expo/cli@npm%3A${version}#~/${file}`,
        },
      });
    }
    if (args[0] === 'install') install(app, committed);
    return '';
  };
}
