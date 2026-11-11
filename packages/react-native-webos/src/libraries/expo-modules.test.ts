import { mkdirSync, mkdtempSync, realpathSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { expoModuleTargetName } from './expo-modules.ts';
import { findNativeLibraries } from './find-native-libraries.ts';

const WEBOS_MODULE = {
  platforms: ['apple', 'android', 'webos'],
  webos: { cmake: 'webos/CMakeLists.txt' },
};

let app: string;

function writeJson(path: string, value: unknown): void {
  mkdirSync(join(path, '..'), { recursive: true });
  writeFileSync(path, JSON.stringify(value));
}

function localModule(dir: string, config: unknown): string {
  writeJson(join(app, dir, 'expo-module.config.json'), config);
  return realpathSync(join(app, dir));
}

beforeEach(() => {
  app = realpathSync(mkdtempSync(join(tmpdir(), 'rn-webos-expo-modules-')));
});

afterEach(() => {
  rmSync(app, { recursive: true, force: true });
});

describe('findNativeLibraries, for Expo modules', () => {
  it('finds a local module under modules/ with its own webOS CMakeLists.txt', () => {
    writeJson(join(app, 'package.json'), { name: 'app' });
    const dir = localModule('modules/server-discovery', WEBOS_MODULE);

    const [library] = findNativeLibraries(app, []).libraries;

    expect(library).toEqual({
      name: 'server-discovery',
      version: '',
      dir,
      targetName: 'expo_server_discovery',
      codegen: null,
      declaredWebOSCMake: join(dir, 'webos', 'CMakeLists.txt'),
    });
  });

  it('skips a local module that does not list webos', () => {
    writeJson(join(app, 'package.json'), { name: 'app' });
    localModule('modules/siri-search', { platforms: ['apple'] });

    expect(findNativeLibraries(app, []).libraries).toEqual([]);
  });

  it("reads the local modules from the app's expo.autolinking.nativeModulesDir", () => {
    writeJson(join(app, 'package.json'), {
      name: 'app',
      expo: { autolinking: { nativeModulesDir: './native' } },
    });
    localModule('native/launcher', WEBOS_MODULE);
    localModule('modules/ignored', WEBOS_MODULE);

    expect(findNativeLibraries(app, []).libraries.map((library) => library.name)).toEqual([
      'launcher',
    ]);
  });

  it('finds a dependency that is a webOS Expo module', () => {
    writeJson(join(app, 'package.json'), { name: 'app', dependencies: { 'expo-beacon': '1' } });
    writeJson(join(app, 'node_modules/expo-beacon/package.json'), {
      name: 'expo-beacon',
      version: '2.0.0',
    });
    writeJson(join(app, 'node_modules/expo-beacon/expo-module.config.json'), WEBOS_MODULE);

    const [library] = findNativeLibraries(app, []).libraries;

    expect(library?.targetName).toBe('expo_expo_beacon');
    expect(library?.version).toBe('2.0.0');
  });

  it('warns about a module that lists webos without a cmake path', () => {
    writeJson(join(app, 'package.json'), { name: 'app' });
    localModule('modules/half', { platforms: ['webos'] });

    const scan = findNativeLibraries(app, []);

    expect(scan.libraries).toEqual([]);
    expect(scan.warnings).toEqual([
      'half: its expo-module.config.json lists webos but no "webos": { "cmake" }; ignored',
    ]);
  });
});

describe('expoModuleTargetName', () => {
  it('turns a scoped npm name into a CMake identifier', () => {
    expect(expoModuleTargetName('@kromatv/lan-beacon')).toBe('expo_kromatv_lan_beacon');
  });
});
