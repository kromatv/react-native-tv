import {
  copyFileSync,
  existsSync,
  mkdirSync,
  mkdtempSync,
  readFileSync,
  rmSync,
  statSync,
  utimesSync,
  writeFileSync,
} from 'node:fs';
import { tmpdir } from 'node:os';
import { dirname, join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { fakeApp } from '../app/app.fixtures.ts';
import { readZip } from '../signing/zip-read.ts';
import { writeZip } from '../signing/zip-write.ts';
import { platformProfile, SUPPORTED_PLATFORMS } from '../tizen/platform-profiles.ts';
import { checkPackagedRuntime } from './packaged-runtime.ts';
import { writeTizenProject } from './tizen-project.ts';

vi.mock('./app-config-file.ts', () => ({ writeAppConfig: vi.fn(async () => {}) }));

const LIBRARY = join('lib', 'libreact-native-tizen.so');
let root: string;

beforeEach(() => {
  root = mkdtempSync(join(tmpdir(), 'rn-tizen-variant-'));
});

afterEach(() => {
  rmSync(root, { recursive: true, force: true });
});

function file(name: string, text: string, modified = new Date()): string {
  const path = join(root, name);
  writeFileSync(path, text);
  utimesSync(path, modified, modified);
  return path;
}

function inputs(runtimeLibrary: string) {
  mkdirSync(join(root, 'assets'), { recursive: true });
  return {
    app: fakeApp(root),
    profile: platformProfile(SUPPORTED_PLATFORMS[0] ?? ''),
    runtimeLibrary,
    bundle: {
      javascript: file('index.js', 'js'),
      bytecode: file('index.hbc', 'hbc'),
      assetsDir: join(root, 'assets'),
    },
    log: null,
    icuData: file('icudt77l.dat', 'icu'),
  };
}

/**
 * The part of a .NET build that decides what the package carries, as
 * Tizen.NET.Sdk 1.2.1's `_TizenPrepareTpkPackage` does: a file is copied into
 * `tpkroot` only when it is newer than the copy there, keeping its time.
 */
function dotnetBuild(projectDir: string): string {
  const from = join(projectDir, LIBRARY);
  const to = join(projectDir, 'bin', 'tpkroot', LIBRARY);
  if (!existsSync(to) || statSync(from).mtimeMs > statSync(to).mtimeMs) {
    mkdirSync(dirname(to), { recursive: true });
    copyFileSync(from, to);
    utimesSync(to, statSync(from).atime, statSync(from).mtime);
  }
  const tpk = join(projectDir, 'bin', 'app.tpk');
  writeFileSync(
    tpk,
    writeZip([{ name: 'lib/libreact-native-tizen.so', directory: false, data: readFileSync(to) }]),
  );
  return tpk;
}

const packaged = (tpk: string) =>
  readZip(readFileSync(tpk))
    .find((entry) => entry.name === 'lib/libreact-native-tizen.so')
    ?.data.toString();

describe('packaging dev, then release', () => {
  it('ships the release runtime even when it was built before the development one', async () => {
    const hourAgo = new Date(Date.now() - 3_600_000);
    const release = file('release.so', 'release runtime', hourAgo);
    const development = file('development.so', 'development runtime');

    const devTpk = dotnetBuild(await writeTizenProject(inputs(development)));
    const devPackaged = packaged(devTpk);
    await checkPackagedRuntime(devTpk, development, 'development');
    const releaseTpk = dotnetBuild(await writeTizenProject(inputs(release)));

    expect(devPackaged).toBe('development runtime');
    expect(packaged(releaseTpk)).toBe('release runtime');
    await expect(checkPackagedRuntime(releaseTpk, release, 'release')).resolves.toBeUndefined();
  });
});

describe('checkPackagedRuntime', () => {
  it('refuses a package that carries the other flavour, or no runtime at all', async () => {
    const tpk = join(root, 'app.tpk');
    writeFileSync(
      tpk,
      writeZip([
        {
          name: 'lib/libreact-native-tizen.so',
          directory: false,
          data: Buffer.from('development'),
        },
      ]),
    );
    const empty = join(root, 'empty.tpk');
    writeFileSync(empty, writeZip([]));

    await expect(
      checkPackagedRuntime(tpk, file('release.so', 'release'), 'release'),
    ).rejects.toThrow('carries another libreact-native-tizen.so than the release one built for it');
    await expect(checkPackagedRuntime(empty, file('any.so', 'x'), 'release')).rejects.toThrow(
      'holds no lib/libreact-native-tizen.so',
    );
  });
});
