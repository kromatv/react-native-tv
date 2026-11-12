import { existsSync, readFileSync, realpathSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { dirname, join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { testApp, type TestApp } from '../app.fixtures.ts';
import { EXPO_CLI_ENTRY, EXPO_CLI_ENTRY_FILE, installExpoCli } from './expo-cli.fixtures.ts';
import { addExpoCliHook, EXPO_CLI_EXTENSION } from './expo-cli-hook.ts';
import { fakePnpm, type ManagerCall } from './fake-managers.fixtures.ts';
import { patchExpoCli } from './patch-expo-cli.ts';
import type { RunManager } from './run-manager.ts';

let app: TestApp;
let calls: ManagerCall[];

const OURS = `+        tvCommands = require.resolve('${EXPO_CLI_EXTENSION}', {\n`;
const CONFIG: ManagerCall = ['pnpm', 'config', 'get', 'patchedDependencies', '--json'];

beforeEach(() => {
  app = testApp();
  calls = [];
  installExpoCli(app);
  app.write('pnpm-lock.yaml', '');
  app.write('package.json', '{\n  "name": "app"\n}\n');
});

afterEach(() => {
  app.remove();
});

describe('patchExpoCli with pnpm', () => {
  it('adds the hook in a directory of its own through pnpm patch, and names what to commit', () => {
    expect(patchExpoCli(app.root, fakePnpm(app, calls))).toEqual({
      state: 'patched',
      version: '58.1.20',
      manager: 'pnpm',
      installDir: app.root,
      changed: ['patches/@expo__cli@58.1.20.patch', 'pnpm-workspace.yaml', 'pnpm-lock.yaml'],
      removed: [],
    });
    const editDir = calls[1]?.[3] ?? '';
    expect(calls).toEqual([
      CONFIG,
      ['pnpm', 'patch', '--edit-dir', editDir, '@expo/cli@58.1.20'],
      ['pnpm', 'patch-commit', editDir],
      CONFIG,
    ]);
    expect(editDir.startsWith(join(tmpdir(), 'rn-tv-expo-patch-'))).toBe(true);
    expect(existsSync(dirname(editDir))).toBe(false);
    expect(readFileSync(join(app.root, EXPO_CLI_ENTRY_FILE), 'utf8')).toBe(
      addExpoCliHook(EXPO_CLI_ENTRY),
    );
    expect(readFileSync(join(app.root, 'package.json'), 'utf8')).toBe('{\n  "name": "app"\n}\n');
  });

  it('leaves an Expo CLI that has the hook already, after asking pnpm for old patches', () => {
    installExpoCli(app, addExpoCliHook(EXPO_CLI_ENTRY));

    expect(patchExpoCli(app.root, fakePnpm(app, calls))).toMatchObject({
      state: 'present',
      changed: [],
      removed: [],
    });
    expect(calls).toEqual([CONFIG]);
  });

  it('has pnpm remove its patches of other versions, and no patch that is not ours', () => {
    const patches = new Map([
      ['@expo/cli@58.1.18', 'patches/@expo__cli@58.1.18.patch'],
      ['@expo/cli@58.1.19', 'patches/@expo__cli@58.1.19.patch'],
      ['@expo/cli@58.1.17', 'patches/theirs.patch'],
      ['left-pad@1.3.0', 'patches/left-pad@1.3.0.patch'],
    ]);
    app.write('patches/@expo__cli@58.1.18.patch', OURS);
    app.write('patches/@expo__cli@58.1.19.patch', OURS);
    app.write('patches/theirs.patch', '+their own change');
    app.write('patches/left-pad@1.3.0.patch', OURS);
    app.write('pnpm-workspace.yaml', "patchedDependencies:\n  '@expo/cli@58.1.18': x\n");

    expect(patchExpoCli(app.root, fakePnpm(app, calls, patches))).toMatchObject({
      state: 'patched',
      changed: ['pnpm-workspace.yaml', 'pnpm-lock.yaml', 'patches/@expo__cli@58.1.20.patch'],
      removed: ['58.1.18', '58.1.19'],
    });
    expect(calls[1]).toEqual(['pnpm', 'patch-remove', '@expo/cli@58.1.18', '@expo/cli@58.1.19']);
    expect([...patches.keys()]).toEqual([
      '@expo/cli@58.1.17',
      'left-pad@1.3.0',
      '@expo/cli@58.1.20',
    ]);
    expect(existsSync(join(app.root, 'patches/theirs.patch'))).toBe(true);
  });

  it('keeps the patch of the Expo CLI installed, and names package.json when pnpm keeps its settings there', () => {
    const patches = new Map([
      ['@expo/cli@58.1.19', 'patches/@expo__cli@58.1.19.patch'],
      ['@expo/cli@58.1.20', 'patches/@expo__cli@58.1.20.patch'],
    ]);
    app.write('patches/@expo__cli@58.1.19.patch', OURS);
    app.write('patches/@expo__cli@58.1.20.patch', OURS);
    installExpoCli(app, addExpoCliHook(EXPO_CLI_ENTRY));

    expect(patchExpoCli(app.root, fakePnpm(app, calls, patches))).toMatchObject({
      state: 'present',
      changed: ['package.json', 'pnpm-lock.yaml'],
      removed: ['58.1.19'],
    });
    expect([...patches.keys()]).toEqual(['@expo/cli@58.1.20']);
  });

  it('names the patch file from the app, whether or not a symlink leads to the app', () => {
    const root = realpathSync(app.root);

    expect(patchExpoCli(root, fakePnpm(app, calls))).toMatchObject({
      installDir: root,
      changed: ['patches/@expo__cli@58.1.20.patch', 'pnpm-workspace.yaml', 'pnpm-lock.yaml'],
    });
  });

  it('names the settings and the lockfile when pnpm lists no patch after its commit', () => {
    const pnpm = fakePnpm(app, calls);
    const listingNone: RunManager = (command, args, cwd) =>
      args[0] === 'config' ? 'null\n' : pnpm(command, args, cwd);

    expect(patchExpoCli(app.root, listingNone)).toMatchObject({
      state: 'patched',
      changed: ['pnpm-workspace.yaml', 'pnpm-lock.yaml'],
    });
  });

  it('commits nothing, and leaves no directory behind, when the copy has another layout', () => {
    const pnpm = fakePnpm(app, calls);
    const withOtherLayout: RunManager = (command, args, cwd) => {
      const printed = pnpm(command, args, cwd);
      if (args[0] === 'patch') {
        writeFileSync(join(args[2] ?? '', 'build/src/index.js'), 'module.exports = {};');
      }
      return printed;
    };

    expect(patchExpoCli(app.root, withOtherLayout)).toEqual({
      state: 'unknown-layout',
      version: '58.1.20',
    });
    expect(calls.map((call) => call[1])).toEqual(['config', 'patch']);
    expect(existsSync(dirname(calls[1]?.[3] ?? ''))).toBe(false);
  });
});
