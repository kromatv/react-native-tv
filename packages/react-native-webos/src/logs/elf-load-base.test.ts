import { mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { elfLoadBase } from './elf-load-base.ts';

let dir = '';

beforeEach(() => {
  dir = mkdtempSync(join(tmpdir(), 'rn-webos-elf-'));
});

afterEach(() => {
  rmSync(dir, { recursive: true, force: true });
});

const PT_LOAD = 1;
const PT_NOTE = 4;

function elf32(segments: readonly (readonly [type: number, vaddr: number])[]): string {
  const file = Buffer.alloc(52 + 32 * segments.length);
  file.writeUInt32BE(0x7f454c46, 0);
  file[4] = 1;
  file[5] = 1;
  file.writeUInt32LE(52, 28);
  file.writeUInt16LE(32, 42);
  file.writeUInt16LE(segments.length, 44);
  segments.forEach(([type, vaddr], index) => {
    file.writeUInt32LE(type, 52 + 32 * index);
    file.writeUInt32LE(vaddr, 52 + 32 * index + 8);
  });
  const path = join(dir, 'elf32');
  writeFileSync(path, Buffer.concat([file, Buffer.alloc(16)]));
  return path;
}

function elf64(segments: readonly (readonly [type: number, vaddr: bigint])[]): string {
  const file = Buffer.alloc(64 + 56 * segments.length);
  file.writeUInt32BE(0x7f454c46, 0);
  file[4] = 2;
  file[5] = 1;
  file.writeBigUInt64LE(64n, 32);
  file.writeUInt16LE(56, 54);
  file.writeUInt16LE(segments.length, 56);
  segments.forEach(([type, vaddr], index) => {
    file.writeUInt32LE(type, 64 + 56 * index);
    file.writeBigUInt64LE(vaddr, 64 + 56 * index + 16);
  });
  const path = join(dir, 'elf64');
  writeFileSync(path, file);
  return path;
}

describe('elfLoadBase', () => {
  it("finds the address a TV's fixed-address executable is linked at", () => {
    const executable = elf32([
      [PT_NOTE, 0x154],
      [PT_LOAD, 0x240000],
      [PT_LOAD, 0x10000],
    ]);

    expect(elfLoadBase(executable)).toBe(0x10000n);
  });

  it('answers 0 for a position-independent 64-bit build, as the simulator runs', () => {
    expect(
      elfLoadBase(
        elf64([
          [PT_LOAD, 0n],
          [PT_LOAD, 0x3f0000n],
        ]),
      ),
    ).toBe(0n);
  });

  it('answers null for a file that is not an ELF file, is cut short or is missing', () => {
    const text = join(dir, 'text');
    writeFileSync(
      text,
      'not an executable, but long enough to hold a whole ELF header of 64 bytes',
    );
    const short = join(dir, 'short');
    writeFileSync(short, Buffer.from([0x7f, 0x45, 0x4c, 0x46, 1, 1]));

    expect(elfLoadBase(text)).toBeNull();
    expect(elfLoadBase(short)).toBeNull();
    expect(elfLoadBase(join(dir, 'missing'))).toBeNull();
    expect(elfLoadBase(elf32([[PT_NOTE, 0x154]]))).toBeNull();
  });
});
