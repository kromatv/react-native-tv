import { mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { findLlvmTools } from '../doctor/find-program.ts';
import { CommandError, runSync } from '../exec.ts';
import { crashSymbolizer, llvmFrameResolver } from './llvm-symbolizer.ts';

vi.mock('../doctor/find-program.ts', () => ({ findLlvmTools: vi.fn() }));
vi.mock('../exec.ts', async (importOriginal) => ({
  ...(await importOriginal<typeof import('../exec.ts')>()),
  runSync: vi.fn(),
}));

const CRASH = 'Native crash: SIGSEGV\n#0 libreact-native-tizen.so+0x10\n#1 libc.so.6+0x20';

let dir: string;
let library: string;

function tools(symbolizer: string | null) {
  vi.mocked(findLlvmTools).mockReturnValue({
    clang: null,
    'clang++': null,
    'llvm-ar': null,
    'llvm-ranlib': null,
    'llvm-strip': null,
    'llvm-symbolizer': symbolizer,
    'ld.lld': null,
  });
}

beforeEach(() => {
  dir = mkdtempSync(join(tmpdir(), 'rn-tizen-symbolizer-'));
  library = join(dir, 'libreact-native-tizen.so');
  writeFileSync(library, '');
  tools('/llvm/bin/llvm-symbolizer');
});

afterEach(() => {
  rmSync(dir, { recursive: true, force: true });
  vi.clearAllMocks();
});

describe('llvmFrameResolver', () => {
  it('names the frames in the library and leaves the others unnamed', () => {
    vi.mocked(runSync).mockReturnValue('rntizen::boom()\n/src/Boom.cpp:3:0:0\n\n');

    const resolve = llvmFrameResolver(library);

    expect(
      resolve?.([
        { library: 'libreact-native-tizen.so', offset: '10' },
        { library: 'libc.so.6', offset: '20' },
      ]),
    ).toEqual(['rntizen::boom() at /src/Boom.cpp:3', null]);
    expect(runSync).toHaveBeenCalledWith(
      expect.arrayContaining(['/llvm/bin/llvm-symbolizer', '--obj', library, '0x10']),
      { timeoutMs: 10_000 },
    );
  });

  it('leaves a frame unnamed when the symbolizer knows nothing of it', () => {
    vi.mocked(runSync).mockReturnValue('??\n??:0:0\n');

    expect(
      llvmFrameResolver(library)?.([{ library: 'libreact-native-tizen.so', offset: '1' }]),
    ).toEqual([null]);
  });

  it('leaves every frame unnamed when the symbolizer fails', () => {
    vi.mocked(runSync).mockImplementation(() => {
      throw new CommandError(['llvm-symbolizer'], 1, '');
    });

    expect(
      llvmFrameResolver(library)?.([{ library: 'libreact-native-tizen.so', offset: '1' }]),
    ).toEqual([null]);
  });

  it('runs nothing for a report with no frame in the library', () => {
    expect(llvmFrameResolver(library)?.([{ library: 'libc.so.6', offset: '1' }])).toEqual([null]);
    expect(runSync).not.toHaveBeenCalled();
  });

  it('is null without llvm-symbolizer or without the library', () => {
    expect(llvmFrameResolver(join(dir, 'missing.so'))).toBeNull();
    tools(null);
    expect(llvmFrameResolver(library)).toBeNull();
  });
});

describe('crashSymbolizer', () => {
  it('rewrites native crash reports only', () => {
    vi.mocked(runSync).mockReturnValue('rntizen::boom()\nBoom.cpp:3:0\n');
    const transform = crashSymbolizer(library);

    expect(transform({ level: 'info', message: 'hello' }).message).toBe('hello');
    expect(transform({ level: 'error', message: CRASH }).message).toContain('rntizen::boom()');
  });

  it('leaves a report as it came when nothing can symbolize it', () => {
    tools(null);

    expect(crashSymbolizer(library)({ level: 'error', message: CRASH }).message).toBe(CRASH);
  });
});
