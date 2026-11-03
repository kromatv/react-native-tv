import { mkdirSync, mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { FixableError } from '../errors.ts';
import { locateGccToolchain, locateRootstrap, locateTizenSdk } from './tizen-sdk.ts';

const TARGET = { arch: 'armv7l', platformVersion: '9.0' } as const;

let root: string;

beforeEach(() => {
  root = mkdtempSync(join(tmpdir(), 'rn-tizen-studio-'));
  writeFileSync(join(root, 'sdk.version'), 'TIZEN_SDK_VERSION=6.1');
});

afterEach(() => {
  rmSync(root, { recursive: true, force: true });
});

describe('locateTizenSdk', () => {
  it('finds Tizen Studio where TIZEN_SDK_DIR points, with its package manager', () => {
    expect(locateTizenSdk({ TIZEN_SDK_DIR: root })).toEqual({
      root,
      packageManager: join(root, 'package-manager', 'package-manager-cli.bin'),
    });
  });

  it('says where to get Tizen Studio when there is none', () => {
    const missing = join(root, 'nowhere');

    expect(() => locateTizenSdk({ TIZEN_SDK_DIR: missing })).toThrow(FixableError);
    expect(() => locateTizenSdk({ TIZEN_SDK_DIR: missing })).toThrow(
      `Tizen Studio not found at ${missing}`,
    );
  });
});

describe('locateRootstrap', () => {
  it("returns the target's installed rootstrap", () => {
    const sdk = locateTizenSdk({ TIZEN_SDK_DIR: root });
    const rootstrap = join(
      root,
      'platforms',
      'tizen-9.0',
      'tizen',
      'rootstraps',
      'tizen-9.0-device.core',
    );
    mkdirSync(rootstrap, { recursive: true });

    expect(locateRootstrap(sdk, TARGET)).toBe(rootstrap);
  });

  it('names the package that installs a missing rootstrap', () => {
    const sdk = locateTizenSdk({ TIZEN_SDK_DIR: root });

    expect(() => locateRootstrap(sdk, TARGET)).toThrow('No Tizen 9.0 rootstrap for armv7l');
    expect(() => locateRootstrap(sdk, TARGET)).toThrow(
      expect.objectContaining({
        hint: expect.stringContaining(
          'install TIZEN-9.0-NativeAppDevelopment-CLI --accept-license',
        ),
      }),
    );
  });
});

describe('locateGccToolchain', () => {
  it('picks the newest GCC for the CPU, comparing versions as numbers', () => {
    const sdk = locateTizenSdk({ TIZEN_SDK_DIR: root });
    for (const name of [
      'arm-linux-gnueabi-gcc-9.2',
      'arm-linux-gnueabi-gcc-14.2',
      'aarch64-linux-gnu-gcc-15.1',
    ]) {
      mkdirSync(join(root, 'tools', name), { recursive: true });
    }

    expect(locateGccToolchain(sdk, 'armv7l')).toBe(
      join(root, 'tools', 'arm-linux-gnueabi-gcc-14.2'),
    );
  });

  it('names the package that installs a missing GCC', () => {
    const sdk = locateTizenSdk({ TIZEN_SDK_DIR: root });

    expect(() => locateGccToolchain(sdk, 'aarch64')).toThrow('No GCC toolchain for aarch64');
    mkdirSync(join(root, 'tools'));
    expect(() => locateGccToolchain(sdk, 'x86_64')).toThrow(
      expect.objectContaining({ hint: expect.stringContaining('NativeToolchain-Gcc-14.2') }),
    );
  });
});
