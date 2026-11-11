import { describe, expect, it } from 'vitest';
import { installedLocales } from './icu-common-data.ts';
import { commonDataHeader, resIndex } from './icu-data.fixtures.ts';
import {
  breakDataFiles,
  icuDataFileName,
  icuLanguages,
  keepsIcuItem,
  trimIcuData,
} from './icu-data.ts';

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

  it('drops the data neither Hermes nor Skia asks for', () => {
    const unused = [
      'lang/fr.res',
      'unit/fr.res',
      'brkitr/sent.brk',
      'brkitr/cjdict.dict',
      'brkitr/de.res',
      'translit/root.res',
      'ibm-37_P100-1995.cnv',
      'unames.icu',
      'rfc3491.spp',
    ];

    expect(unused.some((name) => keepsIcuItem(name, french))).toBe(false);
  });
});

const utf16 = (text: string) => new Uint8Array(Buffer.from(text, 'utf16le'));

function breakData(languages: readonly string[]): string[] {
  const data = {
    header: commonDataHeader(),
    packageName: 'icudt78l',
    items: [
      { name: 'brkitr/el.res', bytes: utf16('\0sent_el.brk\0') },
      { name: 'brkitr/ja.res', bytes: utf16('\0line_cj.brk\0line_loose_cj.brk\0') },
      { name: 'brkitr/root.res', bytes: utf16('\0sent.brk\0title.brk\0cjdict.dict\0') },
      { name: 'ja.res', bytes: utf16('\0not_break_data.brk\0') },
    ],
  };
  return [...breakDataFiles(data, new Set(languages))].sort((left, right) =>
    left < right ? -1 : 1,
  );
}

describe('breakDataFiles', () => {
  it('is the grapheme, word and line rules of the root locale for a language with none of its own', () => {
    expect(breakData(['fr'])).toEqual(['char.brk', 'line.brk', 'word.brk']);
  });

  it("adds the rules a language's own item names, and its dictionary", () => {
    expect(breakData(['ja'])).toEqual([
      'char.brk',
      'cjdict.dict',
      'line.brk',
      'line_cj.brk',
      'line_loose_cj.brk',
      'word.brk',
    ]);
    expect(breakData(['el', 'th'])).toEqual([
      'char.brk',
      'line.brk',
      'sent_el.brk',
      'thaidict.dict',
      'word.brk',
    ]);
  });
});

describe('keepsIcuItem, in the break iteration tree', () => {
  const files = new Set(['char.brk', 'word.brk', 'line.brk']);

  it('keeps root, the index, the language and the break data it is given', () => {
    const kept = [
      'brkitr/root.res',
      'brkitr/res_index.res',
      'brkitr/fr.res',
      'brkitr/char.brk',
      'brkitr/line.brk',
    ];

    expect(kept.every((name) => keepsIcuItem(name, french, files))).toBe(true);
  });

  it('drops the other rules and every dictionary a language does not need', () => {
    const dropped = [
      'brkitr/sent.brk',
      'brkitr/line_cj.brk',
      'brkitr/cjdict.dict',
      'brkitr/ja.res',
    ];

    expect(dropped.some((name) => keepsIcuItem(name, french, files))).toBe(false);
  });
});

describe('trimIcuData', () => {
  it('keeps the break rules text is laid out with', () => {
    const data = {
      header: commonDataHeader(),
      packageName: 'icudt78l',
      items: [
        { name: 'brkitr/char.brk', bytes: new Uint8Array(4) },
        { name: 'brkitr/cjdict.dict', bytes: new Uint8Array(4) },
        { name: 'brkitr/line.brk', bytes: new Uint8Array(4) },
        { name: 'brkitr/res_index.res', bytes: resIndex(['fr', 'ja']) },
        { name: 'brkitr/root.res', bytes: new Uint8Array(4) },
        { name: 'brkitr/sent.brk', bytes: new Uint8Array(4) },
        { name: 'brkitr/word.brk', bytes: new Uint8Array(4) },
      ],
    };

    const trimmed = trimIcuData(data, ['fr']);
    const index = trimmed.items.find((item) => item.name === 'brkitr/res_index.res');

    expect(trimmed.items.map((item) => item.name)).toEqual([
      'brkitr/char.brk',
      'brkitr/line.brk',
      'brkitr/res_index.res',
      'brkitr/root.res',
      'brkitr/word.brk',
    ]);
    expect(installedLocales(index?.bytes ?? new Uint8Array())).toEqual(['fr']);
  });

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
