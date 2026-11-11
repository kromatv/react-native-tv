import { describe, expect, it } from 'vitest';
import { FixableError } from '../errors.ts';
import { readTar, writeTar } from './ustar.ts';

const MTIME = 1_794_400_000;
const file = (path: string, text = 'x', mode = 0o644) => ({ path, mode, data: Buffer.from(text) });
const dir = (path: string) => ({ path, mode: 0o777 });

describe('writeTar', () => {
  it('writes a ustar header per entry: name, octal mode, uid and gid 0, size, mtime, type', () => {
    const tar = writeTar([file('bin/app', 'hello', 0o755)], MTIME);
    const header = tar.subarray(0, 512);

    expect(header.toString('latin1', 0, 8)).toBe('bin/app\0');
    expect(header.toString('latin1', 100, 108)).toBe('0000755\0');
    expect(header.toString('latin1', 108, 116)).toBe('0000000\0');
    expect(header.toString('latin1', 116, 124)).toBe('0000000\0');
    expect(header.toString('latin1', 124, 136)).toBe('00000000005\0');
    expect(Number.parseInt(header.toString('latin1', 136, 147), 8)).toBe(MTIME);
    expect(header.toString('latin1', 156, 157)).toBe('0');
    expect(header.toString('latin1', 257, 265)).toBe('ustar\x0000');
    expect(tar.toString('latin1', 512, 517)).toBe('hello');
  });

  it('sums the header with its checksum field read as spaces', () => {
    const header = writeTar([file('a')], MTIME).subarray(0, 512);
    const stored = Number.parseInt(header.toString('latin1', 148, 154), 8);
    const blanked = Buffer.from(header);
    blanked.fill(0x20, 148, 156);

    expect(stored).toBe(blanked.reduce((sum, byte) => sum + byte, 0));
    expect(header.toString('latin1', 154, 156)).toBe('\0 ');
  });

  it('ends a directory with a slash, pads data to 512 bytes and ends with two empty blocks', () => {
    const tar = writeTar([dir('usr'), file('usr/a', 'abc')], MTIME);

    expect(tar.byteLength).toBe(512 + 512 + 512 + 1024);
    expect(tar.subarray(tar.byteLength - 1024).every((byte) => byte === 0)).toBe(true);
    expect(readTar(tar).map((entry) => [entry.path, entry.type, entry.mode])).toEqual([
      ['usr/', 'directory', 0o777],
      ['usr/a', 'file', 0o644],
    ]);
  });

  it('sorts the entries by path, a directory before what it holds', () => {
    const tar = writeTar([file('b/z'), file('a.txt'), dir('b'), file('b/a'), dir('B')], MTIME);

    expect(readTar(tar).map((entry) => entry.path)).toEqual(['B/', 'a.txt', 'b/', 'b/a', 'b/z']);
  });

  it('splits a path over 100 bytes between prefix and name, with no PAX header', () => {
    const prefix = `usr/palm/applications/com.example.app/res/assets/${'d'.repeat(60)}`;
    const name = `${'f'.repeat(40)}.png`;
    const tar = writeTar([file(`${prefix}/${name}`)], MTIME);

    expect(tar.byteLength).toBe(512 + 512 + 1024);
    expect(tar.toString('latin1', 0, name.length + 1)).toBe(`${name}\0`);
    expect(tar.toString('latin1', 345, 345 + prefix.length + 1)).toBe(`${prefix}\0`);
    expect(readTar(tar)[0]?.path).toBe(`${prefix}/${name}`);
  });

  it('refuses a path ustar cannot hold, and one outside ASCII', () => {
    const tooLong = `assets/${'n'.repeat(101)}.png`;

    expect(() => writeTar([file(tooLong)], MTIME)).toThrow(FixableError);
    expect(() => writeTar([file(tooLong)], MTIME)).toThrow('too long');
    expect(() => writeTar([file('assets/é.png')], MTIME)).toThrow('outside ASCII');
  });
});

describe('readTar', () => {
  it('reads owner, mode, time and bytes back', () => {
    const [entry] = readTar(writeTar([file('a', 'bytes', 0o755)], MTIME));

    expect(entry).toMatchObject({
      path: 'a',
      type: 'file',
      mode: 0o755,
      uid: 0,
      gid: 0,
      mtime: MTIME,
    });
    expect(Buffer.from(entry?.data ?? []).toString()).toBe('bytes');
  });

  it('takes the long name a GNU archive puts in a record before the entry', () => {
    const long = `dir/${'n'.repeat(150)}`;
    const record = Buffer.alloc(1024);
    record.write('././@LongLink', 0, 'latin1');
    record.write(`${(long.length + 1).toString(8).padStart(11, '0')}\0`, 124, 'latin1');
    record.write('L', 156, 'latin1');
    record.write(long, 512, 'latin1');
    const entry = writeTar([file('short')], MTIME);

    expect(readTar(Buffer.concat([record, entry]))[0]?.path).toBe(long);
  });

  it('refuses an entry cut short', () => {
    const tar = writeTar([file('a', 'x'.repeat(600))], MTIME);

    expect(() => readTar(tar.subarray(0, 700))).toThrow('runs past');
  });
});
