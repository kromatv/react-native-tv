import { existsSync, readdirSync, readFileSync } from 'node:fs';
import { mkdir, rename, rm, writeFile } from 'node:fs/promises';
import { join } from 'node:path';
import { z } from 'zod';
import { cacheDir } from '../../src/cache-dir.ts';
import { run } from '../../src/exec.ts';
import { downloadVerified } from '../../src/runtime/download.ts';

/**
 * webosbrew's firmware symbol data: what each library of 14 TV firmware images
 * exports, as `dev-toolbox-cli` ships it (Apache-2.0). Fetched into the cache
 * at a pinned commit, never copied into this repository
 * (docs/reference/webos-native-toolchain.md, section 5).
 */
export const FIRMWARE_DATA_PIN = {
  commit: '2d5a7b6881850d825aa7fdc6c0b5e53c91dc40d3',
  url: 'https://github.com/webosbrew/dev-toolbox-cli/archive/2d5a7b6881850d825aa7fdc6c0b5e53c91dc40d3.tar.gz',
  sha256: 'd434cc38aae195507a19bb17545cc96b62293088ee21087c3a33ccef41e67fcb',
  /** The directory of the archive that holds one directory for each firmware. */
  dataDir: 'common/data',
} as const;

const MAX_LIBRARY_BYTES = 64 * 1024 * 1024;

const infoSchema = z.object({
  version: z.string(),
  ota_id: z.string(),
  release: z.string().regex(/^\d+\.\d+\.\d+$/),
});
const indexSchema = z.record(z.string(), z.string());
const librarySchema = z.object({
  name: z.string(),
  needed: z.array(z.string()),
  symbols: z.array(z.string()),
});

/** What one library of a firmware image exports and needs. */
export interface FirmwareLibrary {
  readonly name: string;
  readonly needed: readonly string[];
  /** `name` or `name@version` for every symbol the library defines. */
  readonly symbols: ReadonlySet<string>;
  /** The names in `symbols`, whatever their version. */
  readonly names: ReadonlySet<string>;
}

/** One TV firmware image: its webOS release and the libraries on it. */
export interface Firmware {
  /** The webOS release, such as `4.4.2`. */
  readonly release: string;
  readonly otaId: string;
  /** The library a SONAME or file name resolves to, or null when the image has none. */
  library(name: string): FirmwareLibrary | null;
}

function loadFirmware(dir: string): Firmware {
  const info = infoSchema.parse(JSON.parse(readFileSync(join(dir, 'info.json'), 'utf8')));
  const index = indexSchema.parse(JSON.parse(readFileSync(join(dir, 'index.json'), 'utf8')));
  const loaded = new Map<string, FirmwareLibrary | null>();
  const load = (file: string): FirmwareLibrary | null => {
    const path = join(dir, file);
    if (!existsSync(path)) return null;
    const text = readFileSync(path, 'utf8');
    if (text.length > MAX_LIBRARY_BYTES) throw new Error(`${path} is larger than 64 MB`);
    const { name, needed, symbols } = librarySchema.parse(JSON.parse(text));
    const names = new Set(symbols.map((symbol) => symbol.split('@', 1)[0] ?? symbol));
    return { name, needed, symbols: new Set(symbols), names };
  };
  return {
    release: info.release,
    otaId: info.ota_id,
    library(name) {
      const file = index[name];
      if (file === undefined) return null;
      if (!loaded.has(file)) loaded.set(file, load(file));
      return loaded.get(file) ?? null;
    },
  };
}

/** Orders releases as numbers: 4.10.0 comes after 4.4.2. */
export function compareReleases(left: string, right: string): number {
  const parts = (release: string) => release.split('.').map(Number);
  const [a, b] = [parts(left), parts(right)];
  for (let index = 0; index < 3; index += 1) {
    const difference = (a[index] ?? 0) - (b[index] ?? 0);
    if (difference !== 0) return difference;
  }
  return 0;
}

/** Every firmware image under `dataDir`, oldest webOS release first. */
export function loadFirmwares(dataDir: string): Firmware[] {
  return readdirSync(dataDir, { withFileTypes: true })
    .filter((entry) => entry.isDirectory() && existsSync(join(dataDir, entry.name, 'info.json')))
    .map((entry) => loadFirmware(join(dataDir, entry.name)))
    .sort((left, right) => compareReleases(left.release, right.release));
}

/**
 * The directory holding the pinned firmware data, downloaded and extracted
 * into the cache on first use: 38 MB to fetch, 320 MB on disk.
 */
export async function ensureFirmwareData(): Promise<string> {
  const root = join(cacheDir(), 'firmware-symbols', FIRMWARE_DATA_PIN.commit);
  if (existsSync(join(root, '.complete'))) return root;
  const archive = join(
    cacheDir(),
    'downloads',
    'firmware-symbols',
    `dev-toolbox-cli-${FIRMWARE_DATA_PIN.commit}.tar.gz`,
  );
  await downloadVerified(FIRMWARE_DATA_PIN.url, FIRMWARE_DATA_PIN.sha256, archive);
  const staging = `${root}.staging-${process.pid}`;
  await rm(staging, { recursive: true, force: true });
  await mkdir(staging, { recursive: true });
  const inArchive = `dev-toolbox-cli-${FIRMWARE_DATA_PIN.commit}/${FIRMWARE_DATA_PIN.dataDir}`;
  const depth = inArchive.split('/').length;
  try {
    await run(['tar', '-xzf', archive, '-C', staging, `--strip-components=${depth}`, inArchive], {
      quiet: true,
    });
    await writeFile(join(staging, '.complete'), '');
    await rm(root, { recursive: true, force: true });
    await rename(staging, root);
  } finally {
    await rm(staging, { recursive: true, force: true });
  }
  return root;
}
