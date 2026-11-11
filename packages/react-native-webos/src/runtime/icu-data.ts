import { existsSync } from 'node:fs';
import { mkdir, readFile, rename, rm, writeFile } from 'node:fs/promises';
import { basename, dirname, join } from 'node:path';
import { FixableError } from '../errors.ts';
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

const TRIM_VERSION = 3;
const BREAK_TREE = 'brkitr';
const LOCALE_TREES = new Set(['', 'coll', 'curr', 'zone', BREAK_TREE]);
const ROOT_BREAK_RULES = ['char.brk', 'word.brk', 'line.brk'];
// ICU breaks words, and for some of these lines, by dictionary; `brkitr/root.res`
// names the file per script (docs/reference/skia-rendering.md, section 5).
const DICTIONARIES: Readonly<Record<string, string>> = {
  ja: 'cjdict.dict',
  zh: 'cjdict.dict',
  th: 'thaidict.dict',
  lo: 'laodict.dict',
  km: 'khmerdict.dict',
  my: 'burmesedict.dict',
};
const BREAK_DATA = /\.(?:brk|dict)$/;
const BREAK_RULES_SUFFIX = '.brk';
const NAME_CHARACTER = /[A-Za-z_]/;
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

function isLocaleItem(file: string, languages: ReadonlySet<string>): boolean {
  const locale = NOT_LOCALES.has(file) ? null : LOCALE_ITEM.exec(file);
  return locale?.[1] !== undefined && locale[1] !== 'root' && keepsLocale(locale[1], languages);
}

function namedBreakRules(resource: Uint8Array): string[] {
  const text = Buffer.from(resource.buffer, resource.byteOffset, resource.byteLength).toString(
    'utf16le',
  );
  const names: string[] = [];
  let end = text.indexOf(BREAK_RULES_SUFFIX);
  while (end !== -1) {
    let start = end;
    while (start > 0 && NAME_CHARACTER.test(text.charAt(start - 1))) start -= 1;
    if (start < end) names.push(text.slice(start, end + BREAK_RULES_SUFFIX.length));
    end = text.indexOf(BREAK_RULES_SUFFIX, end + 1);
  }
  return names;
}

/**
 * The break rules and dictionaries, by file name, that text in `languages` is
 * laid out with: the grapheme, word and line rules of the root locale, the
 * rules each language's own `brkitr` item names, and each language's
 * dictionary.
 */
export function breakDataFiles(data: IcuCommonData, languages: ReadonlySet<string>): Set<string> {
  const files = new Set(ROOT_BREAK_RULES);
  for (const language of languages) {
    const dictionary = DICTIONARIES[language];
    if (dictionary !== undefined) files.add(dictionary);
  }
  for (const item of data.items) {
    if (!item.name.startsWith(`${BREAK_TREE}/`)) continue;
    if (!isLocaleItem(item.name.slice(BREAK_TREE.length + 1), languages)) continue;
    for (const named of namedBreakRules(item.bytes)) files.add(named);
  }
  return files;
}

/**
 * Whether ICU data trimmed to `languages` keeps the item `name`: the shared
 * tables, the normalization and collation data, root, and the main,
 * collation, currency, time-zone-name and break iteration data of each
 * language and its regional variants (a date format's number symbols read the
 * currency data, Skia's text layout the break rules), with the break rules and
 * dictionaries in `breakData` only. Display names, units, spelled-out numbers,
 * transliteration and legacy charsets are what neither Hermes nor Skia asks
 * ICU for.
 */
export function keepsIcuItem(
  name: string,
  languages: ReadonlySet<string>,
  breakData: ReadonlySet<string> = new Set(),
): boolean {
  const slash = name.lastIndexOf('/');
  const tree = slash < 0 ? '' : name.slice(0, slash);
  const file = name.slice(slash + 1);
  if (!LOCALE_TREES.has(tree)) return false;
  if (tree === BREAK_TREE && BREAK_DATA.test(file)) return breakData.has(file);
  if (DROPPED_ITEMS.has(file) || DROPPED_EXTENSIONS.some((extension) => file.endsWith(extension))) {
    return false;
  }
  const locale = NOT_LOCALES.has(file) ? null : LOCALE_ITEM.exec(file);
  return locale?.[1] === undefined || keepsLocale(locale[1], languages);
}

/** `data` with only the items {@link keepsIcuItem} keeps, each tree's index listing only them. */
export function trimIcuData(data: IcuCommonData, languages: readonly string[]): IcuCommonData {
  const kept = new Set(languages);
  const breakData = breakDataFiles(data, kept);
  const items = data.items
    .filter((item) => keepsIcuItem(item.name, kept, breakData))
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

async function readCachedIcuData(full: string): Promise<IcuCommonData> {
  try {
    return readIcuCommonData(new Uint8Array(await readFile(full)));
  } catch (error) {
    await rm(full, { force: true });
    const reason = error instanceof Error ? error.message : String(error);
    throw new FixableError(
      `The ICU data in the cache was unreadable (${reason}), so rn-webos removed it`,
      'Run the command again: it extracts the data again from the verified archive.',
    );
  }
}

async function writeTrimmedIcuData(
  task: Task,
  version: string,
  languages: readonly string[],
  trimmed: string,
): Promise<void> {
  const full = join(cacheDir(), 'icu', version, icuDataFileName(version));
  if (!existsSync(full)) await extractFullIcuData(task, full);
  const data = await readCachedIcuData(full);
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
