import { describe, expect, it } from 'vitest';
import {
  intlLocaleDataSource,
  localesWithoutIntlData,
  parseIntlLocales,
} from './intl-locale-data.ts';

describe('parseIntlLocales', () => {
  it('reads a comma-separated list and skips blanks', () => {
    expect(parseIntlLocales(' en, fr ,,')).toEqual(['en', 'fr']);
  });

  it('reads nothing from an unset variable', () => {
    expect(parseIntlLocales(undefined)).toEqual([]);
  });
});

describe('intlLocaleDataSource', () => {
  it('loads each polyfill before its locale data, the ones a service needs first', () => {
    const source = intlLocaleDataSource(['en', 'fr']);
    const order = [
      '@formatjs/intl-pluralrules/polyfill-force.js',
      '@formatjs/intl-pluralrules/locale-data/fr.js',
      '@formatjs/intl-numberformat/polyfill-force.js',
      '@formatjs/intl-numberformat/locale-data/en.js',
      '@formatjs/intl-relativetimeformat/polyfill-force.js',
      "require('./prefer-device-locale')",
    ].map((needle) => source.indexOf(needle));

    expect(order.every((position) => position >= 0)).toBe(true);
    expect([...order].sort((left, right) => left - right)).toEqual(order);
  });

  it('skips everything on a runtime without Intl', () => {
    expect(intlLocaleDataSource(['en'])).toMatch(/^if \(typeof Intl === 'object'\) \{/);
  });

  it('stays an empty module without locales', () => {
    expect(intlLocaleDataSource([])).toBe('export {};\n');
  });
});

describe('localesWithoutIntlData', () => {
  it('names a locale no polyfill has data for', () => {
    expect(localesWithoutIntlData(['fr', 'xx-Nope'])).toEqual(['xx-Nope']);
  });

  it('names a locale that is not a path inside the polyfill', () => {
    expect(localesWithoutIntlData(['../../fr'])).toEqual(['../../fr']);
  });
});
