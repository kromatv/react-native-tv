import { mkdtempSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { describe, expect, it } from 'vitest';
import { INTL_LOCALES_ENV, UPSTREAM_TRANSFORMER_ENV } from './webos-babel-transformer-env.ts';
import { webosAndroidBranchPlugin } from './webos-android-branch.ts';
import { webosListFocusGuidePlugin } from './webos-list-focus-guide.ts';
import { webosPlatformSelectPlugin } from './webos-platform-select.ts';

const upstream = join(mkdtempSync(join(tmpdir(), 'rn-webos-transformer-')), 'upstream.cjs');
writeFileSync(
  upstream,
  'module.exports = { transform: (args) => args, getCacheKey: () => "upstream-key" };\n',
);
process.env[UPSTREAM_TRANSFORMER_ENV] = upstream;
const transformer = await import('./webos-babel-transformer.ts');

describe('the webOS Babel transformer', () => {
  it('adds the Platform and list plugins to a webOS transform', () => {
    const args = transformer.transform({
      options: { platform: 'webos' },
      plugins: ['metro-plugin'],
    });

    expect(args).toEqual({
      options: { platform: 'webos' },
      plugins: [
        'metro-plugin',
        webosPlatformSelectPlugin,
        webosAndroidBranchPlugin,
        webosListFocusGuidePlugin,
      ],
    });
  });

  it('hands every other platform to the app transformer untouched', () => {
    const args = { options: { platform: 'android' }, plugins: ['metro-plugin'] };

    expect(transformer.transform(args)).toBe(args);
  });

  it('extends the app transformer cache key', () => {
    expect(transformer.getCacheKey()).toBe('upstream-key$rn-webos-transform-4$');
  });

  it('keys the cache by the locales a bundle carries Intl data for', () => {
    process.env[INTL_LOCALES_ENV] = 'en,fr';

    expect(transformer.getCacheKey()).toBe('upstream-key$rn-webos-transform-4$en,fr');

    delete process.env[INTL_LOCALES_ENV];
  });

  it('writes the Intl locale data module for the locales rn-webos names', () => {
    process.env[INTL_LOCALES_ENV] = 'fr';

    const args = transformer.transform({
      filename:
        'node_modules/@kromatv/react-native-webos/react-native-overrides/intl/locale-data.ts',
      src: 'export {};',
      options: { platform: 'webos' },
    });

    expect(args).toMatchObject({ src: expect.stringContaining('locale-data/fr.js') });
    delete process.env[INTL_LOCALES_ENV];
  });

  it('leaves every other module source alone', () => {
    const args = transformer.transform({
      filename: 'src/app.tsx',
      src: 'export const a = 1;',
      options: { platform: 'webos' },
    });

    expect(args).toMatchObject({ src: 'export const a = 1;' });
  });
});
