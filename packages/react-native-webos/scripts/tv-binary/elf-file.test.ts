import { describe, expect, it } from 'vitest';
import { FixableError } from '../../src/errors.ts';
import { parseElf } from './elf-file.ts';
import { buildElf } from './elf.fixtures.ts';

const elf = () =>
  parseElf(
    buildElf({
      interpreter: '/lib/ld-linux.so.3',
      needed: ['libm.so.6', 'libc.so.6'],
      flags: 0x5000200,
      imports: [
        { name: 'memcpy', version: { library: 'libc.so.6', node: 'GLIBC_2.4' } },
        { name: 'pipe2', version: { library: 'libc.so.6', node: 'GLIBC_2.9' } },
        { name: 'sin', version: { library: 'libm.so.6', node: 'GLIBC_2.4' } },
        { name: 'eglSwapBuffers' },
        { name: '__gmon_start__', weak: true },
      ],
    }),
  );

describe('parseElf', () => {
  it('reads the header a TV checks first', () => {
    const file = elf();
    expect(file.is64).toBe(false);
    expect(file.type).toBe(2);
    expect(file.machine).toBe(40);
    expect(file.flags).toBe(0x5000200);
    expect(file.interpreter).toBe('/lib/ld-linux.so.3');
  });

  it('lists the needed libraries in link order', () => {
    expect(elf().needed).toEqual(['libm.so.6', 'libc.so.6']);
  });

  it('binds each import to its version node and library', () => {
    expect(elf().imports).toEqual([
      { name: 'memcpy', version: 'GLIBC_2.4', library: 'libc.so.6', weak: false },
      { name: 'pipe2', version: 'GLIBC_2.9', library: 'libc.so.6', weak: false },
      { name: 'sin', version: 'GLIBC_2.4', library: 'libm.so.6', weak: false },
      { name: 'eglSwapBuffers', version: null, library: null, weak: false },
      { name: '__gmon_start__', version: null, library: null, weak: true },
    ]);
  });

  it('hands out the bytes of a section', () => {
    const file = parseElf(
      buildElf({
        interpreter: '/lib/ld-linux.so.3',
        needed: [],
        flags: 0,
        imports: [],
        sections: [{ name: '.rodata', type: 1, bytes: new TextEncoder().encode('abc\0') }],
      }),
    );
    const rodata = file.sections.find((section) => section.name === '.rodata');
    expect(rodata).toBeDefined();
    if (rodata !== undefined) expect([...file.sectionBytes(rodata)]).toEqual([97, 98, 99, 0]);
  });

  it('refuses a file that is no ELF', () => {
    expect(() => parseElf(new TextEncoder().encode('#!/bin/sh\n'.repeat(10)))).toThrow(
      FixableError,
    );
  });

  it('refuses a truncated file instead of reading past its end', () => {
    const bytes = buildElf({
      interpreter: '/lib/ld-linux.so.3',
      needed: ['libc.so.6'],
      flags: 0,
      imports: [],
    });
    expect(() => parseElf(bytes.subarray(0, bytes.length - 100))).toThrow(FixableError);
  });
});
