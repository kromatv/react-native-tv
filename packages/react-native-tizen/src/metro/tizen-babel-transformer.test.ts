import { mkdtempSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { describe, expect, it } from 'vitest';
import { INTL_LOCALES_ENV, UPSTREAM_TRANSFORMER_ENV } from './tizen-babel-transformer-env.ts';
import { tizenAndroidBranchPlugin } from './tizen-android-branch.ts';
import { tizenPlatformSelectPlugin } from './tizen-platform-select.ts';

const upstream = join(mkdtempSync(join(tmpdir(), 'rn-tizen-transformer-')), 'upstream.cjs');
writeFileSync(
  upstream,
  'module.exports = { transform: (args) => args, getCacheKey: () => "upstream-key" };\n',
);
process.env[UPSTREAM_TRANSFORMER_ENV] = upstream;
const transformer = await import('./tizen-babel-transformer.ts');

describe('the Tizen Babel transformer', () => {
  it('adds the Platform plugins to a Tizen transform', () => {
    const args = transformer.transform({
      options: { platform: 'tizen' },
      plugins: ['metro-plugin'],
    });

    expect(args).toEqual({
      options: { platform: 'tizen' },
      plugins: ['metro-plugin', tizenPlatformSelectPlugin, tizenAndroidBranchPlugin],
    });
  });

  it('hands every other platform to the app transformer untouched', () => {
    const args = { options: { platform: 'android' }, plugins: ['metro-plugin'] };

    expect(transformer.transform(args)).toBe(args);
  });

  it('extends the app transformer cache key', () => {
    expect(transformer.getCacheKey()).toBe('upstream-key$rn-tizen-transform-2$');
  });

  it('keys the cache by the locales a bundle carries Intl data for', () => {
    process.env[INTL_LOCALES_ENV] = 'en,fr';

    expect(transformer.getCacheKey()).toBe('upstream-key$rn-tizen-transform-2$en,fr');

    delete process.env[INTL_LOCALES_ENV];
  });

  it('writes the Intl locale data module for the locales rn-tizen names', () => {
    process.env[INTL_LOCALES_ENV] = 'fr';

    const args = transformer.transform({
      filename:
        'node_modules/@kromatv/react-native-tizen/react-native-overrides/intl/locale-data.ts',
      src: 'export {};',
      options: { platform: 'tizen' },
    });

    expect(args).toMatchObject({ src: expect.stringContaining('locale-data/fr.js') });
    delete process.env[INTL_LOCALES_ENV];
  });

  it('leaves every other module source alone', () => {
    const args = transformer.transform({
      filename: 'src/app.tsx',
      src: 'export const a = 1;',
      options: { platform: 'tizen' },
    });

    expect(args).toMatchObject({ src: 'export const a = 1;' });
  });
});
