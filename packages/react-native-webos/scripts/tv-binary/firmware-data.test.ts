import { mkdirSync, mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { compareReleases, loadFirmwares } from './firmware-data.ts';

let data: string;

function writeFirmware(name: string, release: string, files: Readonly<Record<string, unknown>>) {
  const dir = join(data, name);
  mkdirSync(dir);
  writeFileSync(
    join(dir, 'info.json'),
    JSON.stringify({ version: name, ota_id: `HE_DTV_${name}`, release }),
  );
  for (const [file, content] of Object.entries(files)) {
    writeFileSync(join(dir, file), JSON.stringify(content));
  }
}

beforeEach(() => {
  data = mkdtempSync(join(tmpdir(), 'rn-webos-firmware-'));
});

afterEach(() => {
  rmSync(data, { recursive: true, force: true });
});

describe('loadFirmwares', () => {
  it('orders images by release and resolves a SONAME through the index', () => {
    writeFirmware('b', '4.10.0', { 'index.json': {} });
    writeFirmware('a', '4.4.2', {
      'index.json': { 'libz.so.1': 'libz.so.1.2.8.json' },
      'libz.so.1.2.8.json': {
        name: 'libz.so.1',
        package: 'libz1',
        needed: ['libc.so.6'],
        symbols: ['inflate@ZLIB_1.2', 'zlibVersion'],
      },
    });
    const [first, second] = loadFirmwares(data);
    expect([first?.release, second?.release]).toEqual(['4.4.2', '4.10.0']);
    const zlib = first?.library('libz.so.1');
    expect(zlib?.needed).toEqual(['libc.so.6']);
    expect(zlib?.symbols.has('inflate@ZLIB_1.2')).toBe(true);
    expect([...(zlib?.names ?? [])]).toEqual(['inflate', 'zlibVersion']);
    expect(first?.library('libcurl.so.4')).toBeNull();
  });

  it('refuses a library file that is not in the expected shape', () => {
    writeFirmware('a', '4.4.2', {
      'index.json': { 'libz.so.1': 'libz.json' },
      'libz.json': { name: 'libz.so.1', symbols: 'inflate' },
    });
    expect(() => loadFirmwares(data)[0]?.library('libz.so.1')).toThrow();
  });
});

describe('compareReleases', () => {
  it('compares each number', () => {
    expect(compareReleases('4.10.0', '4.4.2')).toBeGreaterThan(0);
    expect(compareReleases('10.2.0', '9.2.0')).toBeGreaterThan(0);
    expect(compareReleases('5.3.1', '5.3.1')).toBe(0);
  });
});
