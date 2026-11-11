import { mkdirSync, mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { listDevices } from '../webos/devices.ts';
import { locateWebOSSdk } from '../webos/webos-sdk.ts';
import { checkDevices, checkGcc, checkRootstrap } from './webos-checks.ts';

vi.mock('../webos/devices.ts', async (importOriginal) => ({
  ...(await importOriginal<typeof import('../webos/devices.ts')>()),
  listDevices: vi.fn(),
}));

const TARGET = { arch: 'armv7l', platformVersion: '9.0' } as const;
const TV = {
  address: { host: '192.168.1.50', port: 26101 },
  serial: '192.168.1.50:26101',
  name: 'Living room',
  capabilities: { cpuArch: 'armv7', platformVersion: '9.0', profile: 'tv', toolPath: null },
};

let root: string;

beforeEach(() => {
  root = mkdtempSync(join(tmpdir(), 'rn-webos-checks-'));
  writeFileSync(join(root, 'sdk.version'), '');
});

afterEach(() => {
  rmSync(root, { recursive: true, force: true });
  vi.clearAllMocks();
});

describe('checkGcc and checkRootstrap', () => {
  it('pass with the GCC and rootstrap webOS Studio has for the target', async () => {
    const sdk = locateWebOSSdk({ WEBOS_SDK_DIR: root });
    mkdirSync(join(root, 'tools', 'arm-linux-gnueabi-gcc-14.2'), { recursive: true });
    mkdirSync(join(root, 'platforms/webos-9.0/webos/rootstraps/webos-9.0-device.core'), {
      recursive: true,
    });

    expect(await checkGcc(sdk, TARGET)).toMatchObject({
      status: 'ok',
      detail: 'arm-linux-gnueabi-gcc-14.2',
    });
    expect(await checkRootstrap(sdk, TARGET)).toMatchObject({
      status: 'ok',
      detail: 'webos-9.0-device.core, from WEBOS-9.0-NativeAppDevelopment-CLI',
    });
  });

  it('fail with the package to install', async () => {
    const sdk = locateWebOSSdk({ WEBOS_SDK_DIR: root });

    expect(await checkGcc(sdk, TARGET)).toMatchObject({ status: 'fail' });
    expect(await checkRootstrap(sdk, TARGET)).toMatchObject({
      status: 'fail',
      fix: expect.stringContaining('NativeAppDevelopment-CLI'),
    });
  });
});

describe('checkDevices', () => {
  it('names every TV that answers', async () => {
    vi.mocked(listDevices).mockResolvedValue([
      { serial: TV.serial, state: 'online', device: TV },
      { serial: '10.0.0.9:26101', state: 'offline', reason: 'timed out' },
    ]);

    expect(await checkDevices()).toMatchObject({
      status: 'ok',
      detail: 'Living room at 192.168.1.50:26101',
    });
  });

  it('calls a TV without a name a TV', async () => {
    vi.mocked(listDevices).mockResolvedValue([
      { serial: TV.serial, state: 'online', device: { ...TV, name: '' } },
    ]);

    expect((await checkDevices()).detail).toBe('TV at 192.168.1.50:26101');
  });

  it('warns, with how to connect one, when no TV answers', async () => {
    vi.mocked(listDevices)
      .mockResolvedValueOnce([])
      .mockResolvedValueOnce([{ serial: TV.serial, state: 'offline', reason: 'timed out' }]);

    expect(await checkDevices()).toMatchObject({ status: 'warn', detail: 'none connected yet' });
    expect(await checkDevices()).toMatchObject({
      status: 'warn',
      detail: 'none of 1 answers',
      fix: expect.stringContaining('rn-webos devices --connect'),
    });
  });
});
