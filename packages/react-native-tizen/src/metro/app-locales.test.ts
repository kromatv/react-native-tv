import { mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { describe, expect, it } from 'vitest';
import { appIntlLocales } from './app-locales.ts';

describe('appIntlLocales', () => {
  it('leaves the locales to the transformer when the app config cannot be read', () => {
    const dir = mkdtempSync(join(tmpdir(), 'rn-tizen-locales-'));
    writeFileSync(join(dir, 'package.json'), '{"name":"x"}');
    try {
      expect(appIntlLocales(dir)).toBeNull();
    } finally {
      rmSync(dir, { recursive: true, force: true });
    }
  });
});
