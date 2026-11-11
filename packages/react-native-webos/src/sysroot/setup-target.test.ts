import { mkdirSync, mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { recordingTask } from '../fakes.fixtures.ts';
import { RUNTIME_DIR } from '../package-paths.ts';
import { buildCMakeProject } from '../runtime/cmake.ts';
import { locateWebOSSdk } from '../webos/webos-sdk.ts';
import { mirrorSysroot } from './mirror-sysroot.ts';
import { setupTarget } from './setup-target.ts';

vi.mock('../runtime/cmake.ts', async (importOriginal) => ({
  ...(await importOriginal<typeof import('../runtime/cmake.ts')>()),
  buildCMakeProject: vi.fn(),
}));
vi.mock('./mirror-sysroot.ts', () => ({
  mirrorSysroot: vi.fn(async () => ({ path: '/sysroot', patchedLibraries: 2, reused: false })),
}));

const TARGET = { arch: 'aarch64', platformVersion: '9.0' } as const;
let dir: string;

beforeEach(() => {
  dir = mkdtempSync(join(tmpdir(), 'rn-webos-target-'));
  writeFileSync(join(dir, 'sdk.version'), '');
  mkdirSync(join(dir, 'platforms/webos-9.0/webos/rootstraps/webos-9.0-device64.core'), {
    recursive: true,
  });
  vi.stubEnv('RN_WEBOS_CACHE_DIR', join(dir, 'cache'));
});

afterEach(() => {
  rmSync(dir, { recursive: true, force: true });
  vi.clearAllMocks();
  vi.unstubAllEnvs();
});

describe('setupTarget', () => {
  it('builds the glibc compat archive against the raw rootstrap, then mirrors the rootstrap', async () => {
    const sdk = locateWebOSSdk({ WEBOS_SDK_DIR: dir });
    const task = recordingTask();

    const sysroot = await setupTarget(task, sdk, TARGET);

    expect(sysroot.patchedLibraries).toBe(2);
    expect(buildCMakeProject).toHaveBeenCalledWith(
      task,
      expect.objectContaining({
        source: join(RUNTIME_DIR, 'compat'),
        definitions: expect.objectContaining({
          WEBOS_USE_RAW_ROOTSTRAP: 'ON',
          WEBOS_ARCH: 'aarch64',
        }),
      }),
    );
    expect(mirrorSysroot).toHaveBeenCalledWith({
      rootstrap: join(dir, 'platforms/webos-9.0/webos/rootstraps/webos-9.0-device64.core'),
      destination: join(dir, 'cache', 'sysroots', 'webos-9.0-device64.core'),
      compatArchive: expect.stringMatching(/librn-webos-glibc-compat\.a$/),
    });
  });

  it('builds nothing for a target whose rootstrap is not installed', async () => {
    const sdk = locateWebOSSdk({ WEBOS_SDK_DIR: dir });

    await expect(
      setupTarget(recordingTask(), sdk, { ...TARGET, platformVersion: '8.0' }),
    ).rejects.toThrow('No webOS 8.0 rootstrap');
    expect(buildCMakeProject).not.toHaveBeenCalled();
  });
});
