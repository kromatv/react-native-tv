import { beforeEach, describe, expect, it, vi } from 'vitest';
import { findLlvmTools } from '../doctor/find-program.ts';
import { FixableError } from '../errors.ts';
import { runSync } from '../exec.ts';
import { stripExecutable } from './strip-executable.ts';

vi.mock('../doctor/find-program.ts', () => ({ findLlvmTools: vi.fn() }));
vi.mock('../exec.ts', () => ({ runSync: vi.fn(() => '') }));

function tools(strip: string | null) {
  vi.mocked(findLlvmTools).mockReturnValue({
    clang: null,
    'clang++': null,
    'llvm-ar': null,
    'llvm-ranlib': null,
    'llvm-strip': strip,
    'llvm-symbolizer': null,
    'ld.lld': null,
  });
}

beforeEach(() => {
  vi.clearAllMocks();
});

describe('stripExecutable', () => {
  it('strips the file in place with llvm-strip', () => {
    tools('/llvm/bin/llvm-strip');

    stripExecutable('/app/webos/app/react-native-webos');

    expect(runSync).toHaveBeenCalledWith(
      ['/llvm/bin/llvm-strip', '--strip-all', '/app/webos/app/react-native-webos'],
      { timeoutMs: 60_000 },
    );
  });

  it('says how to get llvm-strip when there is none', () => {
    tools(null);

    expect(() => stripExecutable('/app/react-native-webos')).toThrow(FixableError);
    expect(runSync).not.toHaveBeenCalled();
  });
});
