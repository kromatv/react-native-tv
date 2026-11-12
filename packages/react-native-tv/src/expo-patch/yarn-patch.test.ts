import { existsSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { testApp, type TestApp } from '../app.fixtures.ts';
import { EXPO_CLI_ENTRY, EXPO_CLI_ENTRY_FILE, installExpoCli } from './expo-cli.fixtures.ts';
import { addExpoCliHook, EXPO_CLI_EXTENSION } from './expo-cli-hook.ts';
import { fakeYarn, type ManagerCall } from './fake-managers.fixtures.ts';
import { patchExpoCli } from './patch-expo-cli.ts';
import type { RunManager } from './run-manager.ts';

let app: TestApp;
let calls: ManagerCall[];

const OURS = `+        tvCommands = require.resolve('${EXPO_CLI_EXTENSION}', {\n`;
const YARN_4_LOCKFILE = '# Manual changes might be lost\n\n__metadata:\n  version: 10\n';
const PATCH_NEW = '.yarn/patches/@expo-cli-npm-58.1.20-b9cf9fdd52.patch';
const PATCH_OLD = '.yarn/patches/@expo-cli-npm-58.1.19-0a1b2c3d4e.patch';
const NEW = `patch:@expo/cli@npm%3A58.1.20#~/${PATCH_NEW}`;
const OLD = `patch:@expo/cli@npm%3A58.1.19#~/${PATCH_OLD}`;

function resolutions(): unknown {
  return JSON.parse(readFileSync(join(app.root, 'package.json'), 'utf8')).resolutions;
}

beforeEach(() => {
  app = testApp();
  calls = [];
  installExpoCli(app);
  app.write('yarn.lock', YARN_4_LOCKFILE);
  app.write('package.json', '{\n  "name": "app"\n}\n');
});

afterEach(() => {
  app.remove();
});

describe('patchExpoCli with Yarn 2 and later', () => {
  it('adds the hook through yarn patch, commits, installs, and names what to commit', () => {
    expect(patchExpoCli(app.root, fakeYarn(app, calls))).toEqual({
      state: 'patched',
      version: '58.1.20',
      manager: 'yarn',
      installDir: app.root,
      changed: [PATCH_NEW, 'package.json', 'yarn.lock'],
      removed: [],
    });
    expect(calls).toEqual([
      ['yarn', 'patch', '--json', '@expo/cli@npm:58.1.20'],
      ['yarn', 'patch-commit', '-s', expect.stringMatching(/xfs-.*user$/)],
      ['yarn', 'install'],
    ]);
    expect(readFileSync(join(app.root, EXPO_CLI_ENTRY_FILE), 'utf8')).toBe(
      addExpoCliHook(EXPO_CLI_ENTRY),
    );
    expect(resolutions()).toEqual({ '@expo/cli@npm:^58.1.19': NEW });
  });

  it('leaves an Expo CLI that has the hook already, and its patch', () => {
    app.write(PATCH_NEW, OURS);
    app.write('package.json', { resolutions: { '@expo/cli@npm:^58.1.19': NEW } });
    installExpoCli(app, addExpoCliHook(EXPO_CLI_ENTRY));

    expect(patchExpoCli(app.root, fakeYarn(app, calls))).toMatchObject({
      state: 'present',
      changed: [],
      removed: [],
    });
    expect(calls).toEqual([]);
    expect(existsSync(join(app.root, PATCH_NEW))).toBe(true);
  });

  it('removes its patch of another Expo CLI, and nothing else of package.json', () => {
    app.write(PATCH_OLD, OURS);
    app.write(PATCH_NEW, OURS);
    app.write(
      'package.json',
      `{\n\t"name": "app",\n\t"resolutions": {\n\t\t"@expo/cli@npm:^58.1.18": "${OLD}",\n\t\t"@expo/cli@npm:^58.1.19": "${NEW}",\n\t\t"left-pad": "1.3.0"\n\t}\n}\n`,
    );
    installExpoCli(app, addExpoCliHook(EXPO_CLI_ENTRY));

    expect(patchExpoCli(app.root, fakeYarn(app, calls))).toMatchObject({
      state: 'present',
      changed: ['package.json'],
      removed: ['58.1.19'],
    });
    expect(readFileSync(join(app.root, 'package.json'), 'utf8')).toBe(
      `{\n\t"name": "app",\n\t"resolutions": {\n\t\t"@expo/cli@npm:^58.1.19": "${NEW}",\n\t\t"left-pad": "1.3.0"\n\t}\n}\n`,
    );
    expect(existsSync(join(app.root, PATCH_OLD))).toBe(false);
    expect(existsSync(join(app.root, PATCH_NEW))).toBe(true);
  });

  it('removes every patch of ours when the hook is missing, the one for this version too, then patches', () => {
    app.write(PATCH_OLD, OURS);
    app.write(PATCH_NEW, OURS);
    app.write('package.json', {
      resolutions: { '@expo/cli@npm:^58.1.17': OLD, '@expo/cli@npm:^58.1.18': NEW },
    });

    expect(patchExpoCli(app.root, fakeYarn(app, calls))).toMatchObject({
      state: 'patched',
      changed: ['package.json', PATCH_NEW, 'yarn.lock'],
      removed: ['58.1.19'],
    });
    expect(resolutions()).toEqual({ '@expo/cli@npm:^58.1.19': NEW });
    expect(existsSync(join(app.root, PATCH_OLD))).toBe(false);
  });

  it("builds on the app's own patch of Expo CLI, and never removes it", () => {
    const theirs = 'patch:@expo/cli@npm%3A58.1.20#~/.yarn/patches/theirs.patch';
    const elsewhere = 'patch:@expo/cli@npm%3A58.1.19#./patches/elsewhere.patch';
    app.write('.yarn/patches/theirs.patch', '+their own change');
    app.write('patches/elsewhere.patch', OURS);
    app.write('package.json', {
      resolutions: { '@expo/cli@npm:^58.1.18': theirs, '@expo/cli@npm:^58.1.17': elsewhere },
    });

    expect(patchExpoCli(app.root, fakeYarn(app, calls))).toMatchObject({
      state: 'patched',
      removed: [],
    });
    expect(calls[0]).toEqual(['yarn', 'patch', '--update', '--json', '@expo/cli']);
    expect(resolutions()).toMatchObject({
      '@expo/cli@npm:^58.1.18': theirs,
      '@expo/cli@npm:^58.1.17': elsewhere,
    });
    expect(readFileSync(join(app.root, '.yarn/patches/theirs.patch'), 'utf8')).toBe(
      '+their own change',
    );
  });

  it('names only the manifest and the lockfile when it cannot tell which patch file yarn wrote', () => {
    const yarn = fakeYarn(app, calls);
    const elsewhere = 'patch:@expo/cli@npm%3A58.1.20#./patches/elsewhere.patch';
    const committing: RunManager = (command, args, cwd) => {
      if (args[0] === 'patch-commit') app.write('package.json', { resolutions: { a: elsewhere } });
      return args[0] === 'patch' ? yarn(command, args, cwd) : '';
    };

    expect(patchExpoCli(app.root, committing)).toMatchObject({
      state: 'patched',
      changed: ['package.json', 'yarn.lock'],
    });
    rmSync(join(app.root, 'package.json'));
    expect(patchExpoCli(join(app.root, 'clients/tv'), committing)).toMatchObject({
      state: 'patched',
      changed: ['package.json', 'yarn.lock'],
    });
  });

  it('throws what yarn printed when it names no directory, or another version', () => {
    const says =
      (printed: string): RunManager =>
      () =>
        printed;

    expect(() => patchExpoCli(app.root, says('Usage Error: No package found'))).toThrow(
      'yarn patch named no directory to edit:\nUsage Error: No package found',
    );
    expect(() => patchExpoCli(app.root, says('{"locator":"@expo/cli@npm:58.1.20"}'))).toThrow(
      'yarn patch named no directory to edit:',
    );
    expect(() =>
      patchExpoCli(app.root, says('{"locator":"@expo/cli@npm:57.0.34","path":"/tmp/x"}\n')),
    ).toThrow('yarn patch prepared @expo/cli@npm:57.0.34, and this app runs @expo/cli 58.1.20.');
  });

  it('commits nothing when the copy yarn prepared has another layout', () => {
    const yarn = fakeYarn(app, calls);
    const withOtherLayout: RunManager = (command, args, cwd) => {
      const printed = yarn(command, args, cwd);
      const copy: unknown = args[0] === 'patch' ? JSON.parse(printed).path : null;
      if (copy !== null)
        writeFileSync(join(String(copy), 'build/src/index.js'), 'module.exports = {};');
      return printed;
    };

    expect(patchExpoCli(app.root, withOtherLayout)).toEqual({
      state: 'unknown-layout',
      version: '58.1.20',
    });
    expect(calls).toHaveLength(1);
  });
});
