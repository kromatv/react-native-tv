import { afterEach, describe, expect, it, vi } from 'vitest';
import { MissingProgramError } from '../exec.ts';
import { recordingTask } from '../fakes.fixtures.ts';
import { findLlvmTools, findProgram } from './find-program.ts';
import { installedWebOSSdk } from '../sysroot/webos-sdk.ts';
import {
  checkCcache,
  checkCMake,
  checkLlvm,
  checkNinja,
  checkNode,
  checkSdk,
} from './toolchain-checks.ts';

vi.mock('./find-program.ts', () => ({ findLlvmTools: vi.fn(), findProgram: vi.fn() }));
vi.mock('../sysroot/webos-sdk.ts', () => ({ installedWebOSSdk: vi.fn() }));

const LLVM = {
  clang: '/llvm/clang',
  'clang++': '/llvm/clang++',
  'llvm-ar': '/llvm/llvm-ar',
  'llvm-ranlib': '/llvm/llvm-ranlib',
  'llvm-strip': '/llvm/llvm-strip',
  'llvm-symbolizer': '/llvm/llvm-symbolizer',
  'ld.lld': '/llvm/ld.lld',
};

function answering(output: string) {
  return recordingTask(() => output);
}

afterEach(() => {
  vi.clearAllMocks();
});

describe('checkLlvm', () => {
  it('passes with the clang version when every tool is there', async () => {
    vi.mocked(findLlvmTools).mockReturnValue(LLVM);
    const task = answering('Homebrew clang version 21.1.8\nTarget: arm64-apple-darwin');

    expect(await checkLlvm(task)).toEqual({
      title: 'LLVM',
      status: 'ok',
      detail: 'clang 21.1.8, ld.lld',
    });
    expect(task.commands).toEqual([['/llvm/clang', '--version']]);
  });

  it('names the tools that are missing, and passes without llvm-symbolizer', async () => {
    vi.mocked(findLlvmTools).mockReturnValueOnce({ ...LLVM, 'llvm-symbolizer': null });
    expect(await checkLlvm(answering('clang version 23.1.4'))).toMatchObject({ status: 'ok' });
    vi.mocked(findLlvmTools).mockReturnValue({ ...LLVM, 'ld.lld': null, 'llvm-ar': null });

    expect(await checkLlvm(answering(''))).toMatchObject({
      status: 'fail',
      detail: 'missing llvm-ar, ld.lld',
      fix: expect.stringContaining('WEBOS_LLVM_DIR'),
    });
  });
});

describe('checkCMake', () => {
  it('passes a CMake new enough for the runtime', async () => {
    expect(await checkCMake(answering('cmake version 4.4.4\n'))).toMatchObject({
      status: 'ok',
      detail: '4.4.4',
    });
  });

  it('fails an older CMake', async () => {
    expect(await checkCMake(answering('cmake version 3.22.1\n'))).toMatchObject({
      status: 'fail',
      detail: '3.22.1 is older than 3.24',
    });
  });

  it('says how to install a CMake that is missing', async () => {
    const task = recordingTask(() => {
      throw new MissingProgramError('cmake');
    });

    expect(await checkCMake(task)).toMatchObject({
      status: 'fail',
      detail: 'cmake is not installed',
    });
  });
});

describe('checkSdk', () => {
  it('passes with where the SDK is, and only warns before the first build downloaded it', async () => {
    vi.mocked(installedWebOSSdk)
      .mockResolvedValueOnce({
        path: '/cache/sdk/2026.08-webos',
        version: '2026.08-webos',
        reused: true,
      })
      .mockResolvedValueOnce(null);

    expect(await checkSdk()).toEqual({
      title: 'webOS SDK',
      status: 'ok',
      detail: '2026.08-webos at /cache/sdk/2026.08-webos',
    });
    expect(await checkSdk()).toMatchObject({
      status: 'warn',
      detail: '2026.08-webos not downloaded yet',
      fix: expect.stringContaining('rn-webos setup'),
    });
  });
});

describe('checkNinja and checkNode', () => {
  it('report the version each prints', async () => {
    expect(await checkNinja(answering('1.13.2\n'))).toMatchObject({
      status: 'ok',
      detail: '1.13.2',
    });
    expect(await checkNode(answering('v22.13.0\n'))).toMatchObject({
      status: 'ok',
      detail: '22.13.0',
    });
  });

  it("shows a tool's first line when it prints no version", async () => {
    expect(await checkNinja(answering('ninja: unknown\nmore'))).toMatchObject({
      detail: 'ninja: unknown',
    });
  });
});

describe('checkCcache', () => {
  it('passes when ccache is installed, and only warns when it is not', async () => {
    vi.mocked(findProgram).mockReturnValueOnce('/usr/bin/ccache').mockReturnValueOnce(null);

    expect(await checkCcache()).toMatchObject({ status: 'ok' });
    expect(await checkCcache()).toMatchObject({
      status: 'warn',
      detail: 'not installed (optional)',
    });
  });
});
