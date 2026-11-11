import { describe, expect, it } from 'vitest';
import {
  filterInstalledLocales,
  installedLocales,
  readIcuCommonData,
  writeIcuCommonData,
} from './icu-common-data.ts';
import { commonDataHeader, resIndex } from './icu-data.fixtures.ts';

const bytes = (...values: number[]) => new Uint8Array(values);

describe('ICU common data', () => {
  it('reads back every item it wrote, in order, under the package name', () => {
    const data = {
      header: commonDataHeader(),
      packageName: 'icudt78l',
      items: [
        { name: 'coll/root.res', bytes: bytes(1, 2, 3) },
        { name: 'fr.res', bytes: bytes(4) },
        { name: 'root.res', bytes: bytes(5, 6) },
      ],
    };

    const read = readIcuCommonData(writeIcuCommonData(data));

    expect(read.packageName).toBe('icudt78l');
    expect(read.items.map((item) => item.name)).toEqual(['coll/root.res', 'fr.res', 'root.res']);
    expect([...(read.items[1]?.bytes.subarray(0, 1) ?? [])]).toEqual([4]);
  });

  it('aligns every item on 16 bytes, as ICU maps them', () => {
    const data = {
      header: commonDataHeader(),
      packageName: 'icudt78l',
      items: [
        { name: 'a.res', bytes: bytes(1) },
        { name: 'b.res', bytes: bytes(2) },
      ],
    };
    const file = writeIcuCommonData(data);
    const toc = new DataView(file.buffer, 32);

    expect(
      [toc.getUint32(8, true) + 32, toc.getUint32(16, true) + 32].map((at) => at % 16),
    ).toEqual([0, 0]);
  });

  it('refuses a file that is not ICU data', () => {
    expect(() => readIcuCommonData(new Uint8Array(64))).toThrow('Not an ICU data file');
  });
});

describe('filterInstalledLocales', () => {
  it('lists only the locales kept, in their order', () => {
    const index = resIndex(['de', 'en', 'en_GB', 'fr', 'root']);

    const kept = filterInstalledLocales(index, (locale) => locale !== 'de' && locale !== 'en_GB');

    expect(installedLocales(kept)).toEqual(['en', 'fr', 'root']);
  });

  it('leaves the index it was given alone', () => {
    const index = resIndex(['de', 'fr']);

    filterInstalledLocales(index, (locale) => locale === 'fr');

    expect(installedLocales(index)).toEqual(['de', 'fr']);
  });
});
