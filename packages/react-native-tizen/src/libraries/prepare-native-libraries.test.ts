import {
  existsSync,
  mkdirSync,
  mkdtempSync,
  readFileSync,
  realpathSync,
  rmSync,
  statSync,
  writeFileSync,
} from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { findPackage } from '../app/app-packages.ts';
import {
  describeLibraries,
  type LinkedLibrary,
  prepareNativeLibraries,
} from './prepare-native-libraries.ts';

const SPEC = `import type { TurboModule } from 'react-native';
import { TurboModuleRegistry } from 'react-native';

export interface Spec extends TurboModule {
  add(a: number, b: number): number;
}

export default TurboModuleRegistry.getEnforcing<Spec>('Adder');
`;

const LIBRARY: LinkedLibrary = {
  name: 'react-native-svg',
  version: '15.16.0',
  dir: '/app/node_modules/react-native-svg',
  codegenName: 'rnsvg',
  codegenDir: '/cache/libraries/rnsvg',
  implementation: '/runtime/host/src/svg/CMakeLists.txt',
};

let app: string;
let output: string;

beforeEach(() => {
  app = mkdtempSync(join(tmpdir(), 'rn-tizen-app-'));
  output = mkdtempSync(join(tmpdir(), 'rn-tizen-out-'));
  writeFileSync(join(app, 'package.json'), JSON.stringify({ name: 'app' }));
});

afterEach(() => {
  rmSync(app, { recursive: true, force: true });
  rmSync(output, { recursive: true, force: true });
});

describe('prepareNativeLibraries', () => {
  it('lists no library for an app without codegen specs', async () => {
    const reactNative = { name: 'react-native', version: '0.88.0', dir: app };

    const prepared = await prepareNativeLibraries(app, reactNative, output);

    expect(JSON.parse(readFileSync(prepared.file, 'utf8'))).toEqual({ libraries: [] });
  });

  it('leaves libraries.json untouched when nothing changed, so CMake does not reconfigure', async () => {
    const reactNative = { name: 'react-native', version: '0.88.0', dir: app };
    const first = await prepareNativeLibraries(app, reactNative, output);
    const written = statSync(first.file).mtimeMs;

    const second = await prepareNativeLibraries(app, reactNative, output);

    expect(statSync(second.file).mtimeMs).toBe(written);
  });
});

describe('prepareNativeLibraries with a codegen library', () => {
  it("runs the app's codegen over the library's specs and lists it as codegen only", async () => {
    const reactNative = findPackage(process.cwd(), 'react-native');
    if (reactNative === null) throw new Error('react-native is not installed');
    writeFileSync(
      join(app, 'package.json'),
      JSON.stringify({ name: 'app', dependencies: { adder: '1' } }),
    );
    const library = join(app, 'node_modules', 'adder');
    mkdirSync(join(library, 'src'), { recursive: true });
    writeFileSync(
      join(library, 'package.json'),
      JSON.stringify({
        name: 'adder',
        version: '1.0.0',
        codegenConfig: { name: 'AdderSpec', type: 'modules', jsSrcsDir: 'src' },
      }),
    );
    writeFileSync(join(library, 'src', 'NativeAdder.ts'), SPEC);

    const prepared = await prepareNativeLibraries(app, reactNative, output);

    expect(prepared.libraries).toEqual([
      {
        name: 'adder',
        version: '1.0.0',
        dir: realpathSync(library),
        codegenName: 'AdderSpec',
        codegenDir: join(output, 'AdderSpec'),
        implementation: null,
      },
    ]);
    expect(existsSync(join(output, 'AdderSpec', 'AdderSpecJSI.h'))).toBe(true);
  });
});

describe('describeLibraries', () => {
  it('says none for no library', () => {
    expect(describeLibraries([])).toBe('none');
  });

  it('marks a library with no Tizen implementation as codegen only', () => {
    expect(describeLibraries([LIBRARY, { ...LIBRARY, name: 'maps', implementation: null }])).toBe(
      'react-native-svg, maps (codegen only)',
    );
  });
});
