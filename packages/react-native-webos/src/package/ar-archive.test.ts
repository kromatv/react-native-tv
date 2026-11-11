import { describe, expect, it } from 'vitest';
import { readAr, writeAr } from './ar-archive.ts';

const MTIME = 1_794_400_000;

describe('writeAr', () => {
  it('writes the global header, then a 60-byte header and the data of each member', () => {
    const archive = writeAr([{ name: 'debian-binary', data: Buffer.from('2.0\n') }], MTIME);

    expect(archive.toString('latin1', 0, 8)).toBe('!<arch>\n');
    expect(archive.toString('latin1', 8, 68)).toBe(
      `${'debian-binary'.padEnd(16)}${String(MTIME).padEnd(12)}${'0'.padEnd(6)}${'0'.padEnd(6)}${'100644'.padEnd(8)}${'4'.padEnd(10)}\`\n`,
    );
    expect(archive.toString('latin1', 68)).toBe('2.0\n');
  });

  it('pads a member of odd size with one line feed, which the next header follows', () => {
    const archive = writeAr(
      [
        { name: 'odd', data: Buffer.from('abc') },
        { name: 'even', data: Buffer.from('ab') },
      ],
      MTIME,
    );

    expect(archive.byteLength).toBe(8 + 60 + 3 + 1 + 60 + 2);
    expect(archive[8 + 60 + 3]).toBe(0x0a);
    expect(readAr(archive).map((member) => [member.name, member.data.toString()])).toEqual([
      ['odd', 'abc'],
      ['even', 'ab'],
    ]);
  });

  it('refuses a name ar cannot hold bare: one with a slash, a space, or over 16 bytes', () => {
    for (const name of ['data.tar.gz/', 'a b', 'a-name-of-17-char', '']) {
      expect(() => writeAr([{ name, data: Buffer.alloc(0) }], MTIME)).toThrow('not a name');
    }
  });
});

describe('readAr', () => {
  it('reads back every header field', () => {
    const [member] = readAr(writeAr([{ name: 'control.tar.gz', data: Buffer.from('x') }], MTIME));

    expect(member).toMatchObject({
      name: 'control.tar.gz',
      mtime: MTIME,
      uid: 0,
      gid: 0,
      mode: '100644',
    });
  });

  it('keeps the trailing slash GNU ar writes, so a caller can tell', () => {
    const gnu = Buffer.concat([
      Buffer.from('!<arch>\n'),
      Buffer.from(
        `${'debian-binary/'.padEnd(16)}${'0'.padEnd(12)}${'0'.padEnd(6)}${'0'.padEnd(6)}${'100644'.padEnd(8)}${'4'.padEnd(10)}\`\n2.0\n`,
      ),
    ]);

    expect(readAr(gnu)[0]?.name).toBe('debian-binary/');
  });

  it('refuses what is not an ar archive, or is cut short', () => {
    const archive = writeAr([{ name: 'a', data: Buffer.from('abcd') }], MTIME);

    expect(() => readAr(Buffer.from('PK\x03\x04 not ar'))).toThrow('Not an ar archive');
    expect(() => readAr(archive.subarray(0, archive.byteLength - 2))).toThrow('runs past');
    expect(() => readAr(archive.subarray(0, 30))).toThrow('no member header');
  });
});
