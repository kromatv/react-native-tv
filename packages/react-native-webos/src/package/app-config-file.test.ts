import { mkdtempSync, readFileSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, describe, expect, it } from 'vitest';
import { appDir } from '../app/app-dir.fixtures.ts';
import { APP_CONFIG_FILE, writeAppConfig } from './app-config-file.ts';

const dirs: string[] = [];

afterEach(() => {
  for (const dir of dirs.splice(0)) rmSync(dir, { recursive: true, force: true });
});

describe('writeAppConfig', () => {
  it("writes the app's public Expo config where expo-constants reads it", async () => {
    const app = appDir({ name: 'Config', slug: 'config', extra: { api: 'https://example.org' } });
    const res = mkdtempSync(join(tmpdir(), 'rn-tizen-res-'));
    dirs.push(app, res);

    await writeAppConfig(app, res);

    expect(JSON.parse(readFileSync(join(res, APP_CONFIG_FILE), 'utf8'))).toMatchObject({
      name: 'Config',
      extra: { api: 'https://example.org' },
    });
  });
});
