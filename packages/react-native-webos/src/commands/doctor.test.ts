import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { appFloor, checkApp } from '../doctor/app-check.ts';
import { passed, type CheckResult } from '../doctor/check-result.ts';
import * as webosChecks from '../doctor/webos-checks.ts';
import * as toolchainChecks from '../doctor/toolchain-checks.ts';
import { FixableError } from '../errors.ts';
import { captureProcessOutput } from '../fakes.fixtures.ts';
import { locateWebOSSdk } from '../webos/webos-sdk.ts';
import { doctorCommand } from './doctor.ts';

vi.mock('../webos/webos-sdk.ts', () => ({ locateWebOSSdk: vi.fn() }));
vi.mock('../doctor/app-check.ts', () => ({ appFloor: vi.fn(() => undefined), checkApp: vi.fn() }));
vi.mock('../doctor/webos-checks.ts', () => ({
  checkCertificates: vi.fn(),
  checkDevices: vi.fn(),
  checkGcc: vi.fn(),
  checkRootstrap: vi.fn(),
}));
vi.mock('../doctor/toolchain-checks.ts', () => ({
  checkCcache: vi.fn(),
  checkCMake: vi.fn(),
  checkDotnet: vi.fn(),
  checkLlvm: vi.fn(),
  checkNinja: vi.fn(),
  checkNode: vi.fn(),
}));

const SDK = { root: '/webos-studio', packageManager: '/pm' };
let output: ReturnType<typeof captureProcessOutput>;

function everyCheckPasses(): void {
  for (const check of [...Object.values(webosChecks), ...Object.values(toolchainChecks)]) {
    vi.mocked(check).mockResolvedValue(passed('Check', 'fine'));
  }
  vi.mocked(checkApp).mockResolvedValue({
    title: 'App',
    status: 'skip',
    detail: 'no Expo app here',
  });
}

beforeEach(() => {
  output = captureProcessOutput();
  vi.mocked(locateWebOSSdk).mockReturnValue(SDK);
  everyCheckPasses();
  process.exitCode = undefined;
});

afterEach(() => {
  vi.clearAllMocks();
  process.exitCode = undefined;
});

describe('rn-webos doctor', () => {
  it('prints every check and says the machine is ready for the target', async () => {
    await doctorCommand.run(['--arch', 'aarch64', '--project', 'app'], { cwd: '/work' });

    expect(output.stdout()).toContain('webOS Studio      /webos-studio');
    expect(output.stdout()).toContain('Ready to build for webOS 6.0 on aarch64.');
    expect(webosChecks.checkGcc).toHaveBeenCalledWith(SDK, {
      arch: 'aarch64',
      platformVersion: '6.0',
    });
    expect(checkApp).toHaveBeenCalledWith('/work/app');
    expect(process.exitCode).toBeUndefined();
  });

  it('prints the fix under a failed check, counts the failures and exits 1', async () => {
    const failing: CheckResult = {
      title: 'CMake',
      status: 'fail',
      detail: 'old',
      fix: 'brew upgrade cmake',
    };
    vi.mocked(toolchainChecks.checkCMake).mockResolvedValue(failing);
    vi.mocked(toolchainChecks.checkNinja).mockResolvedValue({ ...failing, title: 'Ninja' });
    vi.mocked(toolchainChecks.checkCcache).mockResolvedValue({ ...failing, status: 'warn' });

    await doctorCommand.run([], { cwd: '/work' });

    expect(output.stdout()).toContain('    brew upgrade cmake');
    expect(output.stdout()).toContain('2 checks failed.');
    expect(process.exitCode).toBe(1);
  });

  it('says one check failed', async () => {
    vi.mocked(toolchainChecks.checkNode).mockResolvedValue({
      title: 'Node.js',
      status: 'fail',
      detail: 'none',
    });

    await doctorCommand.run([], { cwd: '/work' });

    expect(output.stdout()).toContain('1 check failed.');
  });

  it('skips the webOS Studio checks when webOS Studio is missing, and fails that one', async () => {
    vi.mocked(locateWebOSSdk).mockImplementation(() => {
      throw new FixableError('webOS Studio not found at /nowhere', 'Install webOS Studio');
    });

    await doctorCommand.run([], { cwd: '/work' });

    expect(webosChecks.checkGcc).not.toHaveBeenCalled();
    expect(output.stdout()).toMatch(/- GCC runtime {7}needs webOS Studio/);
    expect(output.stdout()).toContain(
      'needs webOS Studio, then IOT-Headed-6.0-NativeAppDevelopment-CLI',
    );
    expect(output.stdout()).toContain('webOS Studio not found at /nowhere');
    expect(output.stdout()).toContain('1 check failed.');
  });

  it("checks for the floor the app's platform prop sets, unless --platform says otherwise", async () => {
    vi.mocked(appFloor).mockReturnValue('8.0');

    await doctorCommand.run([], { cwd: '/work' });
    await doctorCommand.run(['--platform', '7.0'], { cwd: '/work' });

    expect(vi.mocked(webosChecks.checkRootstrap).mock.calls.map((call) => call[1])).toEqual([
      { arch: 'armv7l', platformVersion: '8.0' },
      { arch: 'armv7l', platformVersion: '7.0' },
    ]);
    expect(appFloor).toHaveBeenCalledOnce();
  });

  it('lets a bug in locating webOS Studio through', async () => {
    vi.mocked(locateWebOSSdk).mockImplementation(() => {
      throw new TypeError('broken');
    });

    await expect(doctorCommand.run([], { cwd: '/work' })).rejects.toThrow('broken');
  });
});
