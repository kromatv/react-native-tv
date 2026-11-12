import { mkdtempSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { recordingTask } from '../fakes.fixtures.ts';
import { ensureCaBundle } from './ca-bundle.ts';
import { pinnedDependency } from './dependencies.ts';
import { downloadVerified } from './download.ts';

vi.mock('./download.ts', () => ({ downloadVerified: vi.fn() }));

let cache: string;

beforeEach(() => {
  cache = mkdtempSync(join(tmpdir(), 'rn-webos-cacert-'));
  vi.stubEnv('RN_WEBOS_CACHE_DIR', cache);
});

afterEach(() => {
  rmSync(cache, { recursive: true, force: true });
  vi.clearAllMocks();
  vi.unstubAllEnvs();
});

describe('ensureCaBundle', () => {
  it("downloads curl's pinned bundle into the cache, checked against its digest", async () => {
    const pin = pinnedDependency('cacert');

    const file = await ensureCaBundle(recordingTask());

    expect(pin.url).toBe(`https://curl.se/ca/cacert-${pin.version}.pem`);
    expect(file).toBe(join(cache, 'downloads', 'cacert', `cacert-cacert-${pin.version}.pem`));
    expect(downloadVerified).toHaveBeenCalledWith(pin.url, pin.sha256, file);
  });
});
