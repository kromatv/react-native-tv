import { chmodSync, mkdirSync, mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { delimiter, join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { findLlvmTools, findProgram } from './find-program.ts';

let dir: string;

function program(subdir: string, name: string, mode = 0o755): string {
  const path = join(dir, subdir, name);
  mkdirSync(join(dir, subdir), { recursive: true });
  writeFileSync(path, '#!/bin/sh\n');
  chmodSync(path, mode);
  return path;
}

beforeEach(() => {
  dir = mkdtempSync(join(tmpdir(), 'rn-tizen-programs-'));
});

afterEach(() => {
  rmSync(dir, { recursive: true, force: true });
});

describe('findProgram', () => {
  it('looks in the given directories before PATH', () => {
    const own = program('own', 'cmake');
    program('path', 'cmake');

    expect(findProgram('cmake', [join(dir, 'own')], { PATH: join(dir, 'path') })).toBe(own);
  });

  it('finds a program on PATH, skipping empty entries and files it cannot run', () => {
    program('first', 'ninja', 0o644);
    const runnable = program('second', 'ninja');
    const path = ['', join(dir, 'first'), join(dir, 'second')].join(delimiter);

    expect(findProgram('ninja', [], { PATH: path })).toBe(runnable);
  });

  it('is null for a program nowhere to be found, or with no PATH at all', () => {
    expect(findProgram('rn-tizen-nothing', [], { PATH: join(dir, 'empty') })).toBeNull();
    expect(findProgram('rn-tizen-nothing', [], {})).toBeNull();
  });
});

describe('findLlvmTools', () => {
  it('takes every tool from TIZEN_LLVM_DIR first', () => {
    const tools = [
      'clang',
      'clang++',
      'llvm-ar',
      'llvm-ranlib',
      'llvm-strip',
      'llvm-symbolizer',
      'ld.lld',
    ];
    for (const tool of tools) program('llvm', tool);

    const found = findLlvmTools({ TIZEN_LLVM_DIR: join(dir, 'llvm'), PATH: '' });

    expect(Object.values(found)).toEqual(tools.map((tool) => join(dir, 'llvm', tool)));
  });
});
