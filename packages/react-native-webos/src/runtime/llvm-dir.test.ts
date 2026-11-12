import { afterEach, describe, expect, it, vi } from 'vitest';
import { findLlvmTools } from '../doctor/find-program.ts';
import { FixableError } from '../errors.ts';
import { locateLlvmDir, missingLlvmTools } from './llvm-dir.ts';

vi.mock('../doctor/find-program.ts', () => ({ findLlvmTools: vi.fn() }));

function installed(missing: readonly string[]): void {
  const at = (name: string) => (missing.includes(name) ? null : `/llvm/bin/${name}`);
  vi.mocked(findLlvmTools).mockReturnValue({
    clang: at('clang'),
    'clang++': at('clang++'),
    'llvm-ar': at('llvm-ar'),
    'llvm-ranlib': at('llvm-ranlib'),
    'llvm-strip': at('llvm-strip'),
    'llvm-symbolizer': at('llvm-symbolizer'),
    'ld.lld': at('ld.lld'),
  });
}

afterEach(() => {
  vi.clearAllMocks();
});

describe('locateLlvmDir', () => {
  it('is the directory that holds clang, with or without llvm-symbolizer', () => {
    installed(['llvm-symbolizer']);

    expect(locateLlvmDir()).toBe('/llvm/bin');
    expect(missingLlvmTools()).toEqual([]);
  });

  it('names what is missing and how to install it', () => {
    installed(['llvm-ar', 'ld.lld']);

    expect(() => locateLlvmDir()).toThrow(FixableError);
    expect(() => locateLlvmDir()).toThrow('llvm-ar, ld.lld are missing');
  });

  it('fails without clang', () => {
    installed(['clang']);

    expect(() => locateLlvmDir()).toThrow('clang is missing');
  });
});
