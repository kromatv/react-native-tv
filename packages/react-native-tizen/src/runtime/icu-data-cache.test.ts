import { existsSync, mkdirSync, mkdtempSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { dirname, join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { FixableError } from '../errors.ts';
import { recordingTask } from '../fakes.fixtures.ts';
import { pinnedDependency } from './dependencies.ts';
import { downloadVerified } from './download.ts';
import { readIcuCommonData, writeIcuCommonData } from './icu-common-data.ts';
import { ensureIcuData, icuDataFileName } from './icu-data.ts';
import { commonDataHeader, resIndex } from './icu-data.fixtures.ts';

vi.mock('./download.ts', () => ({ downloadVerified: vi.fn() }));

const { version } = pinnedDependency('icu');
let cache: string;
let full: string;

function writeFullData(): void {
  mkdirSync(dirname(full), { recursive: true });
  const data = writeIcuCommonData({
    header: commonDataHeader(),
    packageName: `icudt${version.split('.')[0]}l`,
    items: [
      { name: 'de.res', bytes: new Uint8Array(4) },
      { name: 'fr.res', bytes: new Uint8Array(4) },
      { name: 'res_index.res', bytes: resIndex(['de', 'fr', 'root']) },
    ],
  });
  writeFileSync(full, data);
}

beforeEach(() => {
  cache = mkdtempSync(join(tmpdir(), 'rn-tizen-icu-'));
  full = join(cache, 'icu', version, icuDataFileName(version));
  vi.stubEnv('RN_TIZEN_CACHE_DIR', cache);
});

afterEach(() => {
  rmSync(cache, { recursive: true, force: true });
  vi.clearAllMocks();
  vi.unstubAllEnvs();
});

describe('ensureIcuData', () => {
  it("trims the cached full data to the app's languages, once per language set", async () => {
    writeFullData();
    const task = recordingTask();

    const trimmed = await ensureIcuData(task, ['fr-FR']);
    const again = await ensureIcuData(task, ['fr']);

    expect(again).toBe(trimmed);
    const names = readIcuCommonData(new Uint8Array(readFileSync(trimmed))).items.map(
      (item) => item.name,
    );
    expect(names).toEqual(['fr.res', 'res_index.res']);
    expect(task.commands).toEqual([]);
  });

  it('downloads and extracts the full data the first time', async () => {
    const task = recordingTask((command) => {
      if (command[0] === 'tar') writeFullData();
      return '';
    });

    await ensureIcuData(task, ['de']);

    expect(downloadVerified).toHaveBeenCalled();
    expect(task.commands[0]).toEqual(expect.arrayContaining(['tar', '-xzf']));
    expect(task.progressed).toEqual(['downloading ICU']);
  });

  it('removes cached data it cannot read and says to run again', async () => {
    mkdirSync(dirname(full), { recursive: true });
    writeFileSync(full, 'not ICU data');

    const ensuring = ensureIcuData(recordingTask(), ['de']);

    await expect(ensuring).rejects.toBeInstanceOf(FixableError);
    await expect(ensuring).rejects.toThrow('The ICU data in the cache was unreadable');
    expect(existsSync(full)).toBe(false);
  });
});
