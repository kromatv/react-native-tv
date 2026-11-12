import { existsSync, mkdirSync, mkdtempSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { recordingTask } from '../fakes.fixtures.ts';
import { downloadVerified } from '../runtime/download.ts';
import { SDK_DIRECTORIES, SDK_PIN } from './sdk-pin.ts';
import { ensureWebOSSdk, installedWebOSSdk } from './webos-sdk.ts';

vi.mock('../runtime/download.ts', () => ({ downloadVerified: vi.fn() }));

const ROOT = 'arm-webos-linux-gnueabi_sdk-buildroot';
let cache: string;

function extracting(directories: readonly string[] = SDK_DIRECTORIES) {
  return recordingTask((command) => {
    const into = command[command.indexOf('-C') + 1] ?? '';
    for (const directory of directories) mkdirSync(join(into, directory), { recursive: true });
    return '';
  });
}

beforeEach(() => {
  cache = mkdtempSync(join(tmpdir(), 'rn-webos-sdk-'));
  vi.stubEnv('RN_WEBOS_CACHE_DIR', cache);
});

afterEach(() => {
  rmSync(cache, { recursive: true, force: true });
  vi.clearAllMocks();
  vi.unstubAllEnvs();
});

describe('ensureWebOSSdk', () => {
  it('downloads the pinned archive, checked against its digest, into the cache', async () => {
    const sdk = await ensureWebOSSdk(extracting());

    expect(sdk).toEqual({
      path: join(cache, 'sdk', SDK_PIN.version),
      version: SDK_PIN.version,
      reused: false,
    });
    expect(downloadVerified).toHaveBeenCalledWith(
      SDK_PIN.url,
      SDK_PIN.sha256,
      join(
        cache,
        'downloads',
        'webos-sdk',
        'webos-sdk-arm-webos-linux-gnueabi_sdk-buildroot-aarch64.tar.gz',
      ),
    );
  });

  it('extracts the sysroot, the C++ headers and runtime and the GCC files only, without its top directory', async () => {
    const task = extracting();

    const sdk = await ensureWebOSSdk(task);
    const [command = []] = task.commands;

    expect(command.slice(0, 2)).toEqual(['tar', '-xzf']);
    expect(command).toContain('--strip-components=1');
    expect(command.filter((argument) => argument.startsWith(`${ROOT}/`))).toEqual([
      `${ROOT}/arm-webos-linux-gnueabi/sysroot`,
      `${ROOT}/arm-webos-linux-gnueabi/include`,
      `${ROOT}/arm-webos-linux-gnueabi/lib`,
      `${ROOT}/lib/gcc/arm-webos-linux-gnueabi/16.2.0`,
    ]);
    expect(command.filter((argument) => argument.startsWith('--exclude='))).toEqual(
      ['doc', 'i18n', 'locale', 'man', 'terminfo', 'zoneinfo'].map(
        (name) => `--exclude=${ROOT}/arm-webos-linux-gnueabi/sysroot/usr/share/${name}`,
      ),
    );
    expect(existsSync(join(sdk.path, 'arm-webos-linux-gnueabi', 'sysroot'))).toBe(true);
    expect(task.progressed).toEqual(['downloading', 'extracting']);
  });

  it('reuses what it extracted', async () => {
    await ensureWebOSSdk(extracting());
    const task = recordingTask();

    expect(await ensureWebOSSdk(task)).toMatchObject({ reused: true });
    expect(task.commands).toEqual([]);
    expect(downloadVerified).toHaveBeenCalledTimes(1);
  });

  it('fails, leaving nothing behind, when the archive lacks a directory', async () => {
    const partial = extracting(SDK_DIRECTORIES.slice(0, 2));

    await expect(ensureWebOSSdk(partial)).rejects.toThrow('held no arm-webos-linux-gnueabi/lib');
    expect(existsSync(join(cache, 'sdk', SDK_PIN.version))).toBe(false);
    expect(await installedWebOSSdk()).toBeNull();
  });
});

describe('installedWebOSSdk', () => {
  it('is null until the SDK was extracted, then where it is', async () => {
    expect(await installedWebOSSdk()).toBeNull();

    const sdk = await ensureWebOSSdk(extracting());

    expect(await installedWebOSSdk()).toEqual({ ...sdk, reused: true });
  });
});
