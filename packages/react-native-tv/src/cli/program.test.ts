import { readFileSync, realpathSync } from 'node:fs';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { testApp, type TestApp } from '../app.fixtures.ts';
import { EXPO_CLI_ENTRY, installExpoCli } from '../expo-patch/expo-cli.fixtures.ts';
import { addExpoCliHook, EXPO_CLI_EXTENSION } from '../expo-patch/expo-cli-hook.ts';
import { fakeBun, fakePnpm, fakeYarn } from '../expo-patch/fake-managers.fixtures.ts';
import type { CliIo } from './cli-io.ts';
import { runCli } from './program.ts';
import { USAGE } from './usage.ts';

let app: TestApp;
let out: string;
let err: string;
let scripts: (readonly [string, readonly string[]])[];

function io(overrides: Partial<CliIo> = {}): CliIo {
  return {
    cwd: app.root,
    out: (text) => {
      out += text;
    },
    err: (text) => {
      err += text;
    },
    runScript: (script, args) => {
      scripts.push([script, args]);
      return { status: 5, signal: null };
    },
    runManager: fakeBun(app, []),
    ...overrides,
  };
}

beforeEach(() => {
  app = testApp();
  out = '';
  err = '';
  scripts = [];
});

afterEach(() => {
  app.remove();
});

const AFTER_UPGRADE = 'Run rn-tv expo-patch again after Expo CLI changes version.';
const NO_PLATFORM = 'the run command of each TV platform package this app adds';

function withBothPlatforms(): void {
  app.dependOn('tizen', 'webos', 'old');
  app.platformPackage('tizen', { platform: 'tizen', cli: 'rn-tizen' });
  app.platformPackage('webos', { platform: 'webos', cli: 'rn-webos' });
  app.platformPackage('old', {});
}

describe('rn-tv', () => {
  it("runs a platform's own command line under the platform's name, and exits with its code", () => {
    withBothPlatforms();

    expect(runCli(['tizen', 'simulator', 'run', '--no-logs'], io())).toBe(5);
    expect(scripts).toEqual([
      [
        join(realpathSync(app.root), 'node_modules/tizen/cli.js'),
        ['simulator', 'run', '--no-logs'],
      ],
    ]);
    expect(runCli(['webos'], io())).toBe(5);
    expect(scripts[1]).toEqual([join(realpathSync(app.root), 'node_modules/webos/cli.js'), []]);
  });

  it.each([[[]], [['--help']], [['-h']]])('prints its usage for %j', (argv) => {
    expect(runCli(argv, io())).toBe(0);
    expect(out).toBe(USAGE);
  });

  it.each([[['--version']], [['-v']]])('prints its version for %j', (argv) => {
    const manifest = readFileSync(new URL('../../package.json', import.meta.url), 'utf8');

    expect(runCli(argv, io())).toBe(0);
    expect(`"version": "${out.trim()}"`).toBe(/"version": "[^"]+"/.exec(manifest)?.[0]);
  });

  it('refuses a command it does not have, with its usage', () => {
    expect(runCli(['start', '--port', '1'], io())).toBe(2);
    expect(runCli(['run'], io())).toBe(2);
    expect(err).toBe(`Unknown command: start --port 1\n\n${USAGE}Unknown command: run\n\n${USAGE}`);
  });

  it("runs a platform's own run with the options, as run:<platform> and as run <platform>", () => {
    withBothPlatforms();
    const script = (name: string) => join(realpathSync(app.root), 'node_modules', name, 'cli.js');

    expect(runCli(['run:webos', '--device', 'tv'], io())).toBe(5);
    expect(runCli(['run', 'tizen', '--release'], io())).toBe(5);
    expect(scripts).toEqual([
      [script('webos'), ['run', '--device', 'tv']],
      [script('tizen'), ['run', '--release']],
    ]);
  });

  it('names the platforms the app has when asked for another, or that it has none', () => {
    withBothPlatforms();

    expect(runCli(['run:roku'], io())).toBe(2);
    app.dependOn('old');
    expect(runCli(['run:roku'], io())).toBe(2);
    expect(err).toBe(
      'This app has no platform roku. It has run:tizen, run:webos.\n' +
        'This app depends on no TV platform package that runs roku.\n',
    );
  });

  it('lists the platform packages of the app and what each gives', () => {
    expect(runCli(['platforms'], io())).toBe(0);
    withBothPlatforms();
    expect(runCli(['platforms'], io())).toBe(0);
    expect(out).toBe(
      'This app depends on no TV platform package, such as @kromatv/react-native-tizen or @kromatv/react-native-webos.\n' +
        'old  Metro only: this version names no platform or command to run\n' +
        'tizen  run:tizen\n' +
        'webos  run:webos\n',
    );
  });

  it('patches Expo CLI, names the commands it gained, and says so the second time', () => {
    withBothPlatforms();
    installExpoCli(app);
    app.write('bun.lock', '');

    expect(runCli(['expo-patch'], io())).toBe(0);
    expect(runCli(['expo-patch'], io())).toBe(0);
    expect(out).toBe(
      'Expo CLI 58.1.20 now runs: npx expo run:tizen, npx expo run:webos\n' +
        `Commit patches/@expo%2Fcli@58.1.20.patch, package.json, bun.lock in ${app.root}. ${AFTER_UPGRADE}\n` +
        'Expo CLI 58.1.20 has the patch already: npx expo run:tizen, npx expo run:webos\n',
    );
  });

  it('patches an app that has no platform to run yet', () => {
    installExpoCli(app);
    app.write('bun.lock', '');
    app.write('package.json', {});

    expect(runCli(['expo-patch'], io())).toBe(0);
    expect(out).toContain(
      'Expo CLI 58.1.20 now runs: the run command of each TV platform package this app adds\n',
    );
  });

  it('says which patch of an older Expo CLI it removed', () => {
    installExpoCli(app, addExpoCliHook(EXPO_CLI_ENTRY));
    app.write('bun.lock', '');
    app.write('patches/old.patch', EXPO_CLI_EXTENSION);
    app.write('package.json', {
      patchedDependencies: { '@expo/cli@58.1.19': 'patches/old.patch' },
    });

    expect(runCli(['expo-patch'], io())).toBe(0);
    expect(out).toBe(
      `Expo CLI 58.1.20 has the patch already: ${NO_PLATFORM}\n` +
        'Removed the patch of Expo CLI 58.1.19, which is no longer installed.\n' +
        `Commit package.json in ${app.root}. ${AFTER_UPGRADE}\n`,
    );
  });

  it('says what pnpm needs before an Expo upgrade, when it patches and when asked again', () => {
    installExpoCli(app);
    app.write('pnpm-lock.yaml', '');
    const runManager = fakePnpm(app, []);

    expect(runCli(['expo-patch'], io({ runManager }))).toBe(0);
    expect(runCli(['expo-patch'], io({ runManager }))).toBe(0);
    expect(out).toBe(
      `Expo CLI 58.1.20 now runs: ${NO_PLATFORM}\n` +
        `Commit patches/@expo__cli@58.1.20.patch, pnpm-workspace.yaml, pnpm-lock.yaml in ${app.root}. ` +
        'pnpm refuses to install while a patch is for a version that is gone, unless allowUnusedPatches is set: before an Expo upgrade run pnpm patch-remove @expo/cli@58.1.20, and rn-tv expo-patch after it.\n' +
        `Expo CLI 58.1.20 has the patch already: ${NO_PLATFORM}\n` +
        'If pnpm refuses to install because this patch is unused, an Expo upgrade is waiting: run pnpm patch-remove @expo/cli@58.1.20, install, then rn-tv expo-patch.\n',
    );
  });

  it('names the patch file, the manifest and the lockfile Yarn changed', () => {
    installExpoCli(app);
    app.write('yarn.lock', '__metadata:\n  version: 10\n');
    app.write('package.json', {});

    expect(runCli(['expo-patch'], io({ runManager: fakeYarn(app, []) }))).toBe(0);
    expect(out).toBe(
      `Expo CLI 58.1.20 now runs: ${NO_PLATFORM}\n` +
        `Commit .yarn/patches/@expo-cli-npm-58.1.20-b9cf9fdd52.patch, package.json, yarn.lock in ${app.root}. ${AFTER_UPGRADE}\n`,
    );
  });

  it.each([
    [
      'package-lock.json',
      '',
      'npm has no patch feature, so the postinstall script applies the patch after an install. npm install <package> runs no postinstall: run rn-tv expo-patch again after Expo CLI changes version.',
    ],
    [
      'yarn.lock',
      '# yarn lockfile v1\n',
      'Yarn 1 has no patch feature, so the postinstall script applies the patch after every install.',
    ],
  ])('with %s, says why package.json gained a postinstall, once', (lockfile, content, why) => {
    installExpoCli(app);
    app.write(lockfile, content);
    app.write('package.json', {});

    expect(runCli(['expo-patch'], io())).toBe(0);
    installExpoCli(app);
    expect(runCli(['expo-patch'], io())).toBe(0);
    expect(out).toBe(
      `Expo CLI 58.1.20 now runs: ${NO_PLATFORM}\n` +
        `Commit package.json in ${app.root}: ${why}\n` +
        `Expo CLI 58.1.20 now runs: ${NO_PLATFORM}\n`,
    );
  });

  it('says that an install from the workspace root does not run the postinstall of the app', () => {
    installExpoCli(app);
    app.write('package-lock.json', '');
    app.write('clients/tv/package.json', {});
    const cwd = join(app.root, 'clients/tv');

    expect(runCli(['expo-patch'], io({ cwd }))).toBe(0);
    expect(out).toContain(
      `Commit ${join('clients/tv/package.json')} in ${app.root}: npm has no patch feature`,
    );
    expect(out).toContain(
      ` An install from ${app.root} runs the postinstall of that directory only: run rn-tv expo-patch here after it.\n`,
    );
  });

  it('says why it cannot patch, and that run works without', () => {
    expect(runCli(['expo-patch'], io())).toBe(1);
    installExpoCli(app);
    expect(runCli(['expo-patch'], io())).toBe(1);
    app.write('package-lock.json', '');
    installExpoCli(app, 'module.exports = {};');
    expect(runCli(['expo-patch'], io())).toBe(1);
    expect(err).toBe(
      'This app has no Expo CLI: install expo first.\n' +
        'This app has no lockfile, so no package manager to patch Expo CLI with. rn-tv run:<platform> works without the patch.\n' +
        'Expo CLI 58.1.20 is laid out in a way this version of @kromatv/react-native-tv does not know. rn-tv run:<platform> works without the patch.\n',
    );
  });

  it('says why and succeeds all the same as a postinstall script, so an install does not fail on it', () => {
    const env = { npm_lifecycle_event: 'postinstall' };
    const runManager = () => {
      throw new Error('pnpm failed');
    };

    expect(runCli(['expo-patch'], io({ env }))).toBe(0);
    installExpoCli(app, 'module.exports = {};');
    app.write('package-lock.json', '');
    expect(runCli(['expo-patch'], io({ env }))).toBe(0);
    installExpoCli(app);
    app.write('pnpm-lock.yaml', '');
    expect(runCli(['expo-patch'], io({ env: { npm_lifecycle_event: 'build' }, runManager }))).toBe(
      1,
    );
    expect(err).toContain('This app has no Expo CLI: install expo first.\n');
    expect(err).toContain('is laid out in a way this version');
  });

  it('prints what a package manager said when its command fails', () => {
    installExpoCli(app);
    app.write('pnpm-lock.yaml', '');
    const runManager = () => {
      throw new Error('pnpm config get patchedDependencies --json failed:\nERR_PNPM_NO_WORKSPACE');
    };

    expect(runCli(['expo-patch'], io({ runManager }))).toBe(1);
    expect(err).toBe('pnpm config get patchedDependencies --json failed:\nERR_PNPM_NO_WORKSPACE\n');
  });
});
