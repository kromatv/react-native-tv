import { mkdirSync, mkdtempSync, realpathSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { findNativeLibraries } from './find-native-libraries.ts';

const SPECS = { type: 'all', jsSrcsDir: 'src/specs' } as const;

let app: string;

function writePackage(dir: string, manifest: Record<string, unknown>): string {
  mkdirSync(dir, { recursive: true });
  writeFileSync(join(dir, 'package.json'), JSON.stringify(manifest));
  return realpathSync(dir);
}

function dependency(name: string, manifest: Record<string, unknown> = {}): string {
  return writePackage(join(app, 'node_modules', name), { name, version: '1.0.0', ...manifest });
}

function namesFound(excluded: readonly string[] = []): string[] {
  return findNativeLibraries(app, excluded).libraries.map((library) => library.name);
}

beforeEach(() => {
  app = mkdtempSync(join(tmpdir(), 'rn-tizen-libraries-'));
});

afterEach(() => {
  rmSync(app, { recursive: true, force: true });
});

describe('findNativeLibraries', () => {
  it('finds a library the app reaches through another package', () => {
    writePackage(app, { name: 'app', dependencies: { ui: '1' } });
    dependency('ui', { peerDependencies: { 'native-svg': '*' } });
    dependency('native-svg', { codegenConfig: { name: 'nativesvg', ...SPECS } });

    expect(namesFound()).toEqual(['native-svg']);
  });

  it('reads the devDependencies of the app and of no other package', () => {
    writePackage(app, { name: 'app', devDependencies: { tools: '1' } });
    dependency('tools', {
      codegenConfig: { name: 'tools', ...SPECS },
      devDependencies: { 'tools-test': '1' },
    });
    dependency('tools-test', { codegenConfig: { name: 'toolstest', ...SPECS } });

    expect(namesFound()).toEqual(['tools']);
  });

  it('skips an optional peer dependency', () => {
    writePackage(app, { name: 'app', dependencies: { ui: '1' } });
    dependency('ui', {
      peerDependencies: { maybe: '*' },
      peerDependenciesMeta: { maybe: { optional: true } },
    });
    dependency('maybe', { codegenConfig: { name: 'maybe', ...SPECS } });

    expect(namesFound()).toEqual([]);
  });

  it("leaves out the app's react-native", () => {
    writePackage(app, { name: 'app', dependencies: { 'react-native': '1' } });
    const reactNative = dependency('react-native', {
      codegenConfig: { name: 'FBReactNativeSpec', ...SPECS },
    });

    expect(namesFound([reactNative])).toEqual([]);
  });

  it("takes a package's own Tizen CMakeLists.txt from its tizen field", () => {
    writePackage(app, { name: 'app', dependencies: { camera: '1' } });
    const camera = dependency('camera', {
      codegenConfig: { name: 'camera', ...SPECS },
      tizen: { cmake: 'tizen/CMakeLists.txt' },
    });

    const [library] = findNativeLibraries(app, []).libraries;

    expect(library?.declaredTizenCMake).toBe(join(camera, 'tizen', 'CMakeLists.txt'));
  });

  it('ignores, with a warning, a tizen field without a cmake path', () => {
    writePackage(app, { name: 'app', dependencies: { camera: '1' } });
    dependency('camera', { codegenConfig: { name: 'camera', ...SPECS }, tizen: { cmake: 3 } });

    const scan = findNativeLibraries(app, []);

    expect(scan.libraries[0]?.declaredTizenCMake).toBeNull();
    expect(scan.warnings).toEqual(['camera: its "tizen" field needs a "cmake" path; ignored']);
  });

  it('skips, with a warning, a codegenConfig codegen cannot read', () => {
    writePackage(app, { name: 'app', dependencies: { legacy: '1' } });
    dependency('legacy', { codegenConfig: { libraries: [] } });

    const scan = findNativeLibraries(app, []);

    expect(scan.libraries).toEqual([]);
    expect(scan.warnings).toEqual([
      "legacy: its codegenConfig is not one React Native's codegen reads; skipped",
    ]);
  });

  it('keeps the copy nearest the app when two copies share a codegen name', () => {
    writePackage(app, { name: 'app', dependencies: { svg: '2', icons: '1' } });
    const nearest = dependency('svg', {
      version: '2.0.0',
      codegenConfig: { name: 'svg', ...SPECS },
    });
    const icons = dependency('icons', { dependencies: { svg: '1' } });
    writePackage(join(icons, 'node_modules', 'svg'), {
      name: 'svg',
      version: '1.0.0',
      codegenConfig: { name: 'svg', ...SPECS },
    });

    const scan = findNativeLibraries(app, []);

    expect(scan.libraries.map((library) => library.dir)).toEqual([nearest]);
    expect(scan.warnings).toHaveLength(1);
  });
});
