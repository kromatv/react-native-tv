import { describe, expect, it } from 'vitest';
import { isElfSharedObject, rebindStrayLocalSymbols } from './elf-dynamic-symbols.ts';
import { BINDING, bindings, sharedObject } from './elf.fixtures.ts';

const { LOCAL, GLOBAL, WEAK } = BINDING;

describe('rebindStrayLocalSymbols', () => {
  it.each([
    ['32-bit', false],
    ['64-bit', true],
  ])('makes the LOCAL symbols past the global boundary WEAK in a %s library', (_label, is64) => {
    const elf = sharedObject(is64, [LOCAL, LOCAL, GLOBAL, LOCAL, GLOBAL], 2);

    expect(rebindStrayLocalSymbols(elf)).toBe(1);
    expect(bindings(elf, is64, 5)).toEqual([LOCAL, LOCAL, GLOBAL, WEAK, GLOBAL]);
  });

  it('changes nothing in a library without stray locals', () => {
    const elf = sharedObject(false, [LOCAL, GLOBAL, GLOBAL], 1);

    expect(rebindStrayLocalSymbols(elf)).toBe(0);
  });

  it('leaves anything that is not a little-endian ELF alone', () => {
    const bigEndian = sharedObject(false, [GLOBAL, LOCAL], 1);
    bigEndian[5] = 2;

    expect(rebindStrayLocalSymbols(new TextEncoder().encode('INPUT(libc.so.6)'))).toBe(0);
    expect(rebindStrayLocalSymbols(bigEndian)).toBe(0);
  });
});

describe('isElfSharedObject', () => {
  it('is true for a shared object and false for an executable or a linker script', () => {
    const executable = sharedObject(true, [GLOBAL], 0);
    executable[0x10] = 2;

    expect(isElfSharedObject(sharedObject(true, [GLOBAL], 0))).toBe(true);
    expect(isElfSharedObject(executable)).toBe(false);
    expect(isElfSharedObject(new TextEncoder().encode('/* GNU ld script */'.padEnd(80)))).toBe(
      false,
    );
  });
});
