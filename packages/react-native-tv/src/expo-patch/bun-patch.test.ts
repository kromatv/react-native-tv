import { existsSync, readFileSync, realpathSync, rmSync } from 'node:fs';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { testApp, type TestApp } from '../app.fixtures.ts';
import { EXPO_CLI_ENTRY, EXPO_CLI_ENTRY_FILE, installExpoCli } from './expo-cli.fixtures.ts';
import { addExpoCliHook, EXPO_CLI_EXTENSION } from './expo-cli-hook.ts';
import { fakeBun, type ManagerCall } from './fake-managers.fixtures.ts';
import { patchExpoCli } from './patch-expo-cli.ts';
import type { RunManager } from './run-manager.ts';

let app: TestApp;
let calls: ManagerCall[];

const OURS = `+        tvCommands = require.resolve('${EXPO_CLI_EXTENSION}', {\n`;

function manifest(): string {
  return readFileSync(join(app.root, 'package.json'), 'utf8');
}

beforeEach(() => {
  app = testApp();
  calls = [];
  installExpoCli(app);
  app.write('bun.lock', '');
  app.write('package.json', '{\n  "name": "app"\n}\n');
});

afterEach(() => {
  app.remove();
});

describe('patchExpoCli with Bun', () => {
  it('adds the hook through bun patch, in the directory of the lockfile, and names what to commit', () => {
    expect(patchExpoCli(app.root, fakeBun(app, calls))).toEqual({
      state: 'patched',
      version: '58.1.20',
      manager: 'bun',
      installDir: app.root,
      changed: ['patches/@expo%2Fcli@58.1.20.patch', 'package.json', 'bun.lock'],
      removed: [],
    });
    expect(readFileSync(join(app.root, EXPO_CLI_ENTRY_FILE), 'utf8')).toBe(
      addExpoCliHook(EXPO_CLI_ENTRY),
    );
    expect(calls).toEqual([
      ['bun', 'patch', '@expo/cli@58.1.20'],
      ['bun', 'patch', '--commit', 'node_modules/@expo/cli'],
    ]);
  });

  it('edits the directory bun names, which may be absolute', () => {
    const dir = join(realpathSync(app.root), 'node_modules/@expo/cli');
    const bun = fakeBun(app, calls, `bun patch --commit '${dir}'`);

    expect(patchExpoCli(app.root, bun).state).toBe('patched');
    expect(calls[1]).toEqual(['bun', 'patch', '--commit', dir]);
  });

  it('leaves an Expo CLI that has the hook already', () => {
    installExpoCli(app, addExpoCliHook(EXPO_CLI_ENTRY));

    expect(patchExpoCli(app.root, fakeBun(app, calls))).toMatchObject({
      state: 'present',
      changed: [],
      removed: [],
    });
    expect(calls).toEqual([]);
  });

  it('throws what bun printed when it names no directory to edit', () => {
    expect(() => patchExpoCli(app.root, fakeBun(app, calls, 'error: package not found'))).toThrow(
      'bun patch named no directory to edit:\nerror: package not found',
    );
  });

  it('commits nothing when the copy bun prepared has another layout', () => {
    app.write('elsewhere/build/src/index.js', 'module.exports = {};');
    const bun = fakeBun(app, calls, "bun patch --commit 'elsewhere'");

    expect(patchExpoCli(app.root, bun)).toEqual({ state: 'unknown-layout', version: '58.1.20' });
    expect(calls).toHaveLength(1);
  });

  it('removes its patch of another Expo CLI, entry and file, and nothing else of package.json', () => {
    app.write('patches/@expo%2Fcli@58.1.19.patch', OURS);
    app.write('patches/left-pad@1.3.0.patch', '+left');
    app.write(
      'package.json',
      '{\n    "name": "app",\n    "patchedDependencies": {\n        "@expo/cli@58.1.19": "patches/@expo%2Fcli@58.1.19.patch",\n        "left-pad@1.3.0": "patches/left-pad@1.3.0.patch"\n    },\n    "private": true\n}\n',
    );
    installExpoCli(app, addExpoCliHook(EXPO_CLI_ENTRY));

    expect(patchExpoCli(app.root, fakeBun(app, calls))).toMatchObject({
      state: 'present',
      changed: ['package.json'],
      removed: ['58.1.19'],
    });
    expect(manifest()).toBe(
      '{\n    "name": "app",\n    "patchedDependencies": {\n        "left-pad@1.3.0": "patches/left-pad@1.3.0.patch"\n    },\n    "private": true\n}\n',
    );
    expect(existsSync(join(app.root, 'patches/@expo%2Fcli@58.1.19.patch'))).toBe(false);
    expect(existsSync(join(app.root, 'patches/left-pad@1.3.0.patch'))).toBe(true);
  });

  it('removes the old patch, its emptied directory and entries, then patches the Expo CLI installed', () => {
    app.write('patches/@expo%2Fcli@58.1.19.patch', OURS);
    app.write('package.json', {
      patchedDependencies: { '@expo/cli@58.1.19': 'patches/@expo%2Fcli@58.1.19.patch' },
    });

    expect(patchExpoCli(app.root, fakeBun(app, calls))).toMatchObject({
      state: 'patched',
      changed: ['package.json', 'patches/@expo%2Fcli@58.1.20.patch', 'bun.lock'],
      removed: ['58.1.19'],
    });
    expect(JSON.parse(manifest())).toEqual({
      patchedDependencies: { '@expo/cli@58.1.20': 'patches/@expo%2Fcli@58.1.20.patch' },
    });
    expect(existsSync(join(app.root, 'patches/@expo%2Fcli@58.1.19.patch'))).toBe(false);
  });

  it('never touches a patch that is not ours, one outside the app, or one an entry still uses', () => {
    const entries = {
      '@expo/cli@58.1.17': 'patches/theirs.patch',
      '@expo/cli@58.1.18': '../outside.patch',
      '@expo/cli@58.1.19': 'patches/shared.patch',
      '@expo/cli@58.1.20': 'patches/shared.patch',
      '@expo/cli@58.1.16': join(app.root, 'patches/absolute.patch'),
      '@expo/cli@58.1.15': 'patches/missing.patch',
    };
    app.write('patches/theirs.patch', '+their own change');
    app.write('patches/shared.patch', OURS);
    app.write('patches/absolute.patch', OURS);
    app.write('package.json', { patchedDependencies: entries });
    installExpoCli(app, addExpoCliHook(EXPO_CLI_ENTRY));

    expect(patchExpoCli(app.root, fakeBun(app, calls))).toMatchObject({ removed: ['58.1.19'] });
    expect(JSON.parse(manifest()).patchedDependencies).toEqual({
      ...entries,
      '@expo/cli@58.1.19': undefined,
    });
    expect(readFileSync(join(app.root, 'patches/theirs.patch'), 'utf8')).toBe('+their own change');
    expect(existsSync(join(app.root, 'patches/shared.patch'))).toBe(true);
    expect(existsSync(join(app.root, 'patches/absolute.patch'))).toBe(true);
  });

  it('removes nothing and names no patch file where the install directory has no package.json to read', () => {
    rmSync(join(app.root, 'package.json'));
    const bun = fakeBun(app, calls);
    const committing: RunManager = (command, args, cwd) =>
      args[1] === '--commit' ? '' : bun(command, args, cwd);

    expect(patchExpoCli(join(app.root, 'clients/tv'), committing)).toMatchObject({
      state: 'patched',
      installDir: app.root,
      changed: ['package.json', 'bun.lock'],
      removed: [],
    });
  });
});
