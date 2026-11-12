import { deflateRawSync } from 'node:zlib';
import { describe, expect, it } from 'vitest';
import { readZip } from './zip-read.ts';
import { writeZip } from './zip-write.ts';

describe('zip', () => {
  it('reads back what it writes, names in UTF-8 and directories included', () => {
    const entries = [
      { name: 'bin', directory: true, data: Buffer.alloc(0) },
      { name: 'bin/App.dll', directory: false, data: Buffer.alloc(70_000, 3) },
      { name: 'res/ünï.txt', directory: false, data: Buffer.from('text') },
    ];

    expect(readZip(writeZip(entries))).toEqual(entries);
  });

  it('refuses an entry that climbs out of the archive', () => {
    const zip = writeZip([{ name: '../evil', directory: false, data: Buffer.from('x') }]);

    expect(() => readZip(zip)).toThrow('refusing the entry name');
  });

  it('refuses what is not a zip', () => {
    expect(() => readZip(Buffer.alloc(100))).toThrow('not a zip file');
  });

  it('deflates files with raw deflate', () => {
    const zip = writeZip([{ name: 'a.txt', directory: false, data: Buffer.alloc(1000, 1) }]);

    expect(zip.includes(deflateRawSync(Buffer.alloc(1000, 1)))).toBe(true);
  });
});

describe('readZip refusals', () => {
  function withField(offset: number, value: number): Buffer {
    const zip = writeZip([{ name: 'a.txt', directory: false, data: Buffer.from('x') }]);
    const central = zip.indexOf(Buffer.from([0x50, 0x4b, 0x01, 0x02]));
    zip.writeUInt16LE(value, central + offset);
    return zip;
  }

  it('refuses an encrypted entry and a compression method it does not know', () => {
    expect(() => readZip(withField(8, 1))).toThrow('is encrypted');
    expect(() => readZip(withField(10, 12))).toThrow('unsupported compression method 12');
  });
});
