import { mkdtempSync, readFileSync, rmSync, statSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import {
  describeLibraries,
  type LinkedLibrary,
  prepareNativeLibraries,
} from './prepare-native-libraries.ts';

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
