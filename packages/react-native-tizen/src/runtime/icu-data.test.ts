import { describe, expect, it } from 'vitest';
import { installedLocales } from './icu-common-data.ts';
import { commonDataHeader, resIndex } from './icu-data.fixtures.ts';
import { icuDataFileName, icuLanguages, keepsIcuItem, trimIcuData } from './icu-data.ts';

const french = new Set(['fr']);

describe('icuLanguages', () => {
  it('keeps the language of each tag once, lowercased and sorted', () => {
    expect(icuLanguages(['pt-BR', 'fr', 'fr-CA', 'EN'])).toEqual(['en', 'fr', 'pt']);
  });
});

describe('keepsIcuItem', () => {
  it('keeps a language and its regional variants in the main, collation, currency and zone trees', () => {
    expect(
      ['fr.res', 'fr_CA.res', 'coll/fr.res', 'curr/fr.res', 'zone/fr_BE.res'].map((name) =>
        keepsIcuItem(name, french),
      ),
    ).toEqual([true, true, true, true, true]);
  });

  it('drops every other language', () => {
    expect(
      ['de.res', 'coll/de.res', 'zone/frr.res'].some((name) => keepsIcuItem(name, french)),
    ).toBe(false);
  });

  it('keeps root, the indexes, the pools and the shared tables', () => {
    const shared = [
      'root.res',
      'coll/root.res',
      'res_index.res',
      'pool.res',
      'coll/ucadata.icu',
      'zoneinfo64.res',
      'supplementalData.res',
      'nfc.nrm',
    ];

    expect(shared.every((name) => keepsIcuItem(name, french))).toBe(true);
  });

  it('drops the data Hermes never asks for', () => {
    const unused = [
      'lang/fr.res',
      'unit/fr.res',
      'brkitr/word.brk',
      'translit/root.res',
      'ibm-37_P100-1995.cnv',
      'unames.icu',
      'rfc3491.spp',
    ];

    expect(unused.some((name) => keepsIcuItem(name, french))).toBe(false);
  });
});

describe('trimIcuData', () => {
  it('keeps the chosen languages and lists only them in each index', () => {
    const data = {
      header: commonDataHeader(),
      packageName: 'icudt78l',
      items: [
        { name: 'coll/res_index.res', bytes: resIndex(['de', 'fr', 'root']) },
        { name: 'de.res', bytes: new Uint8Array(4) },
        { name: 'fr.res', bytes: new Uint8Array(4) },
        { name: 'res_index.res', bytes: resIndex(['de', 'fr', 'fr_CA', 'root']) },
      ],
    };

    const trimmed = trimIcuData(data, ['fr']);
    const index = trimmed.items.find((item) => item.name === 'res_index.res');

    expect(trimmed.items.map((item) => item.name)).toEqual([
      'coll/res_index.res',
      'fr.res',
      'res_index.res',
    ]);
    expect(installedLocales(index?.bytes ?? new Uint8Array())).toEqual(['fr', 'fr_CA', 'root']);
  });
});

describe('icuDataFileName', () => {
  it('names the file after the major version, little-endian', () => {
    expect(icuDataFileName('78.3')).toBe('icudt78l.dat');
  });
});
