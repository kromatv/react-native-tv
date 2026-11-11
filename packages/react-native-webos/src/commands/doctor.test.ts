import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { checkApp } from '../doctor/app-check.ts';
import { passed, type CheckResult } from '../doctor/check-result.ts';
import * as webosChecks from '../doctor/webos-checks.ts';
import * as toolchainChecks from '../doctor/toolchain-checks.ts';
import { captureProcessOutput } from '../fakes.fixtures.ts';
import { doctorCommand } from './doctor.ts';

vi.mock('../doctor/app-check.ts', () => ({ checkApp: vi.fn() }));
vi.mock('../doctor/webos-checks.ts', () => ({ checkDevices: vi.fn() }));
vi.mock('../doctor/toolchain-checks.ts', () => ({
  checkCcache: vi.fn(),
  checkCMake: vi.fn(),
  checkLlvm: vi.fn(),
  checkNinja: vi.fn(),
  checkNode: vi.fn(),
  checkSdk: vi.fn(),
}));

let output: ReturnType<typeof captureProcessOutput>;

function everyCheckPasses(): void {
  for (const check of [...Object.values(webosChecks), ...Object.values(toolchainChecks)]) {
    vi.mocked(check).mockResolvedValue(passed('Check', 'fine'));
  }
  vi.mocked(toolchainChecks.checkSdk).mockResolvedValue(
    passed('webOS SDK', '2026.08-webos at /cache/sdk'),
  );
  vi.mocked(checkApp).mockResolvedValue({
    title: 'App',
    status: 'skip',
    detail: 'no Expo app here',
  });
}

beforeEach(() => {
  output = captureProcessOutput();
  everyCheckPasses();
  process.exitCode = undefined;
});

afterEach(() => {
  vi.clearAllMocks();
  process.exitCode = undefined;
});

describe('rn-webos doctor', () => {
  it('prints every check and says the machine is ready', async () => {
    await doctorCommand.run(['--project', 'app'], { cwd: '/work' });

    expect(output.stdout()).toContain('webOS SDK         2026.08-webos at /cache/sdk');
    expect(output.stdout()).toContain('Ready to build for webOS TVs.');
    expect(webosChecks.checkDevices).toHaveBeenCalledOnce();
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

  it('no longer takes a CPU or a platform version: every TV build is the same', async () => {
    await expect(doctorCommand.run(['--arch', 'aarch64'], { cwd: '/work' })).rejects.toThrow();
  });
});
