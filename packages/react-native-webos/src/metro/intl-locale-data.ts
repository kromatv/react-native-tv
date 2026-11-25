import { existsSync } from 'node:fs';
import { fileURLToPath } from 'node:url';

/** The module the webOS Babel transformer fills with a bundle's Intl polyfills and locale data. */
export const INTL_LOCALE_DATA_MODULE = 'react-native-overrides/intl/locale-data.ts';

/** The services FormatJS adds on top of Hermes' own Intl, each after those it needs. */
export const INTL_POLYFILLS = [
  '@formatjs/intl-pluralrules',
  '@formatjs/intl-numberformat',
  '@formatjs/intl-relativetimeformat',
] as const;

/** The default when an app names no locales. */
export const DEFAULT_INTL_LOCALES = ['en'] as const;

/** The locales `RN_WEBOS_INTL_LOCALES` names, comma separated. */
export function parseIntlLocales(value: string | undefined): string[] {
  return (value ?? '')
    .split(',')
    .map((locale) => locale.trim())
    .filter((locale) => locale !== '');
}

function hasData(polyfill: string, locale: string): boolean {
  try {
    return existsSync(fileURLToPath(import.meta.resolve(`${polyfill}/locale-data/${locale}.js`)));
  } catch {
    return false;
  }
}

function fallbacks(locale: string): string[] {
  const subtags = locale.split('-');
  return subtags.map((_, dropped) => subtags.slice(0, subtags.length - dropped).join('-'));
}

// FormatJS files a language's default region under the language: no `pt-BR.js`,
// since `pt.js` is Brazilian and registers `pt-BR` itself.
function intlDataLocale(polyfill: string, locale: string): string | undefined {
  return fallbacks(locale).find((candidate) => hasData(polyfill, candidate));
}

function dataLocales(polyfill: string, locales: readonly string[]): string[] {
  return [...new Set(locales.map((locale) => intlDataLocale(polyfill, locale) ?? locale))];
}

/**
 * The source of {@link INTL_LOCALE_DATA_MODULE} for `locales`: each polyfill
 * and its data for every locale, then the device's locale as their default.
 * A runtime built without Intl skips it all. Empty for no locales.
 */
export function intlLocaleDataSource(locales: readonly string[]): string {
  if (locales.length === 0) return 'export {};\n';
  const requires = INTL_POLYFILLS.flatMap((polyfill) => [
    `  require('${polyfill}/polyfill-force.js');`,
    ...dataLocales(polyfill, locales).map(
      (locale) => `  require('${polyfill}/locale-data/${locale}.js');`,
    ),
  ]);
  return [
    "if (typeof Intl === 'object') {",
    ...requires,
    "  require('./prefer-device-locale').preferDeviceLocale();",
    '}',
    'export {};',
    '',
  ].join('\n');
}

/** The locales of `locales` some polyfill has no data for, theirs or their language's. */
export function localesWithoutIntlData(locales: readonly string[]): string[] {
  return locales.filter((locale) =>
    INTL_POLYFILLS.some((polyfill) => intlDataLocale(polyfill, locale) === undefined),
  );
}
