import { chmodSync, mkdirSync, mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { recordingTask } from '../fakes.fixtures.ts';
import { ensureHermesHostCompilers } from '../runtime/hermes-host.ts';
import { locateHermesc } from './hermesc.ts';

vi.mock('../runtime/hermes-host.ts', () => ({
  ensureHermesHostCompilers: vi.fn(async () => ({
    version: '0.1',
    importFile: '/hermes/ImportHostCompilers.cmake',
    hermesc: '/hermes/bin/hermesc',
  })),
}));

let dir: string;

function reactNativeWithCompiler(prebuilt: boolean) {
  const reactNative = join(dir, 'node_modules', 'react-native');
  const compiler = join(dir, 'node_modules', 'hermes-compiler');
  mkdirSync(reactNative, { recursive: true });
  writeFileSync(join(reactNative, 'package.json'), '{"name":"react-native","version":"0.88.0"}');
  mkdirSync(compiler, { recursive: true });
  writeFileSync(join(compiler, 'package.json'), '{"name":"hermes-compiler","version":"0.88.0"}');
  if (prebuilt) {
    for (const platform of ['osx-bin', 'linux64-bin']) {
      mkdirSync(join(compiler, 'hermesc', platform), { recursive: true });
      writeFileSync(join(compiler, 'hermesc', platform, 'hermesc'), '');
      chmodSync(join(compiler, 'hermesc', platform, 'hermesc'), 0o755);
    }
  }
  return { name: 'react-native', version: '0.88.0', dir: reactNative };
}

beforeEach(() => {
  dir = mkdtempSync(join(tmpdir(), 'rn-tizen-hermesc-'));
});

afterEach(() => {
  rmSync(dir, { recursive: true, force: true });
  vi.clearAllMocks();
});

describe('locateHermesc', () => {
  it.runIf(process.platform === 'darwin' || process.platform === 'linux')(
    "takes the hermesc react-native's hermes-compiler ships for this machine",
    async () => {
      const hermesc = await locateHermesc(recordingTask(), reactNativeWithCompiler(true));

      expect(hermesc).toMatch(/hermes-compiler\/hermesc\/(osx|linux64)-bin\/hermesc$/);
      expect(ensureHermesHostCompilers).not.toHaveBeenCalled();
    },
  );

  it('builds the pinned Hermes when hermes-compiler has no hermesc for this machine', async () => {
    const hermesc = await locateHermesc(recordingTask(), reactNativeWithCompiler(false));

    expect(hermesc).toBe('/hermes/bin/hermesc');
  });

  it('builds the pinned Hermes when react-native comes without hermes-compiler', async () => {
    const reactNative = join(dir, 'react-native');
    mkdirSync(reactNative);
    writeFileSync(join(reactNative, 'package.json'), '{"name":"react-native","version":"0.88.0"}');

    const hermesc = await locateHermesc(recordingTask(), {
      name: 'react-native',
      version: '0.88.0',
      dir: reactNative,
    });

    expect(hermesc).toBe('/hermes/bin/hermesc');
  });
});
