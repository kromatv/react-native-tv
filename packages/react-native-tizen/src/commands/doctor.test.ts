import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { checkApp } from '../doctor/app-check.ts';
import { passed, type CheckResult } from '../doctor/check-result.ts';
import * as tizenChecks from '../doctor/tizen-checks.ts';
import * as toolchainChecks from '../doctor/toolchain-checks.ts';
import { FixableError } from '../errors.ts';
import { captureProcessOutput } from '../fakes.fixtures.ts';
import { locateTizenSdk } from '../tizen/tizen-sdk.ts';
import { doctorCommand } from './doctor.ts';

vi.mock('../tizen/tizen-sdk.ts', () => ({ locateTizenSdk: vi.fn() }));
vi.mock('../doctor/app-check.ts', () => ({ checkApp: vi.fn() }));
vi.mock('../doctor/tizen-checks.ts', () => ({
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

const SDK = { root: '/tizen-studio', packageManager: '/pm' };
let output: ReturnType<typeof captureProcessOutput>;

function everyCheckPasses(): void {
  for (const check of [...Object.values(tizenChecks), ...Object.values(toolchainChecks)]) {
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
  vi.mocked(locateTizenSdk).mockReturnValue(SDK);
  everyCheckPasses();
  process.exitCode = undefined;
});

afterEach(() => {
  vi.clearAllMocks();
  process.exitCode = undefined;
});

describe('rn-tizen doctor', () => {
  it('prints every check and says the machine is ready for the target', async () => {
    await doctorCommand.run(['--arch', 'aarch64', '--project', 'app'], { cwd: '/work' });

    expect(output.stdout()).toContain('Tizen Studio      /tizen-studio');
    expect(output.stdout()).toContain('Ready to build for Tizen 9.0 on aarch64.');
    expect(tizenChecks.checkGcc).toHaveBeenCalledWith(SDK, {
      arch: 'aarch64',
      platformVersion: '9.0',
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

  it('skips the Tizen Studio checks when Tizen Studio is missing, and fails that one', async () => {
    vi.mocked(locateTizenSdk).mockImplementation(() => {
      throw new FixableError('Tizen Studio not found at /nowhere', 'Install Tizen Studio');
    });

    await doctorCommand.run([], { cwd: '/work' });

    expect(tizenChecks.checkGcc).not.toHaveBeenCalled();
    expect(output.stdout()).toMatch(/- GCC runtime {7}needs Tizen Studio/);
    expect(output.stdout()).toContain('Tizen Studio not found at /nowhere');
    expect(output.stdout()).toContain('1 check failed.');
  });

  it('lets a bug in locating Tizen Studio through', async () => {
    vi.mocked(locateTizenSdk).mockImplementation(() => {
      throw new TypeError('broken');
    });

    await expect(doctorCommand.run([], { cwd: '/work' })).rejects.toThrow('broken');
  });
});
