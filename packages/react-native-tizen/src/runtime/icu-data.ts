import { existsSync } from 'node:fs';
import { mkdir, readFile, rename, writeFile } from 'node:fs/promises';
import { basename, dirname, join } from 'node:path';
import { cacheDir } from '../cache-dir.ts';
import type { Task } from '../task.ts';
import { archiveFileName, pinnedDependency } from './dependencies.ts';
import { downloadVerified } from './download.ts';
import {
  filterInstalledLocales,
  type IcuCommonData,
  readIcuCommonData,
  writeIcuCommonData,
} from './icu-common-data.ts';

const TRIM_VERSION = 2;
const LOCALE_TREES = new Set(['', 'coll', 'curr', 'zone']);
const DROPPED_EXTENSIONS = ['.cnv', '.spp', '.cfu'];
const DROPPED_ITEMS = new Set([
  'unames.icu',
  'cnvalias.icu',
  'uts46.nrm',
  'nfkc_cf.nrm',
  'nfkc_scf.nrm',
]);
const LOCALE_ITEM = /^(root|[a-z]{2,3}(?:_[A-Za-z0-9]+)*)\.res$/;
const NOT_LOCALES = new Set(['res_index.res', 'pool.res']);

/** The language subtags of `locales`, lowercased and sorted: what ICU data is kept by. */
export function icuLanguages(locales: readonly string[]): string[] {
  const languages = locales.map((locale) => locale.split(/[-_]/)[0]?.toLowerCase() ?? '');
  return [...new Set(languages.filter((language) => /^[a-z]{2,3}$/.test(language)))].sort(
    (left, right) => left.localeCompare(right, 'en'),
  );
}

function keepsLocale(locale: string, languages: ReadonlySet<string>): boolean {
  return locale === 'root' || languages.has(locale.split('_')[0] ?? '');
}

/**
 * Whether ICU data trimmed to `languages` keeps the item `name`: the shared
 * tables, the normalization and collation data, root, and the main,
 * collation, currency and time-zone-name data of each language and its
 * regional variants (a date format's number symbols read the currency data).
 * Display names, units, spelled-out numbers, transliteration, break iteration
 * and legacy charsets are what Hermes never asks ICU for.
 */
export function keepsIcuItem(name: string, languages: ReadonlySet<string>): boolean {
  const slash = name.lastIndexOf('/');
  const tree = slash < 0 ? '' : name.slice(0, slash);
  const file = name.slice(slash + 1);
  if (!LOCALE_TREES.has(tree)) return false;
  if (DROPPED_ITEMS.has(file) || DROPPED_EXTENSIONS.some((extension) => file.endsWith(extension))) {
    return false;
  }
  const locale = NOT_LOCALES.has(file) ? null : LOCALE_ITEM.exec(file);
  return locale?.[1] === undefined || keepsLocale(locale[1], languages);
}

/** `data` with only the items {@link keepsIcuItem} keeps, each tree's index listing only them. */
export function trimIcuData(data: IcuCommonData, languages: readonly string[]): IcuCommonData {
  const kept = new Set(languages);
  const items = data.items
    .filter((item) => keepsIcuItem(item.name, kept))
    .map((item) =>
      item.name.endsWith('res_index.res')
        ? {
            ...item,
            bytes: filterInstalledLocales(item.bytes, (locale) => keepsLocale(locale, kept)),
          }
        : item,
    );
  return { ...data, items };
}

/** The file ICU looks for in its data directory, `icudt<major>l.dat`. */
export function icuDataFileName(version: string): string {
  return `icudt${version.split('.')[0]}l.dat`;
}

async function writeAtomically(path: string, bytes: Uint8Array): Promise<void> {
  const partial = `${path}.partial-${process.pid}`;
  await writeFile(partial, bytes);
  await rename(partial, path);
}

async function extractFullIcuData(task: Task, full: string): Promise<void> {
  const pin = pinnedDependency('icu');
  const archive = join(cacheDir(), 'downloads', 'icu', archiveFileName('icu', pin));
  task.progress('downloading ICU');
  await downloadVerified(pin.url, pin.sha256, archive);
  await mkdir(dirname(full), { recursive: true });
  const member = `icu/source/data/in/${basename(full)}`;
  await task.run(['tar', '-xzf', archive, '-C', dirname(full), '--strip-components=4', member]);
}

async function writeTrimmedIcuData(
  task: Task,
  version: string,
  languages: readonly string[],
  trimmed: string,
): Promise<void> {
  const full = join(cacheDir(), 'icu', version, icuDataFileName(version));
  if (!existsSync(full)) await extractFullIcuData(task, full);
  const data = readIcuCommonData(new Uint8Array(await readFile(full)));
  await mkdir(dirname(trimmed), { recursive: true });
  await writeAtomically(trimmed, writeIcuCommonData(trimIcuData(data, languages)));
}

/**
 * The ICU data an app with `locales` ships, trimmed from the pinned ICU's
 * full data and kept in the cache by language set. Returns its path.
 */
export async function ensureIcuData(task: Task, locales: readonly string[]): Promise<string> {
  const { version } = pinnedDependency('icu');
  const languages = icuLanguages(locales);
  const dir = join(cacheDir(), 'icu', version, `trim${TRIM_VERSION}`, languages.join('-'));
  const trimmed = join(dir, icuDataFileName(version));
  if (!existsSync(trimmed)) await writeTrimmedIcuData(task, version, languages, trimmed);
  return trimmed;
}
