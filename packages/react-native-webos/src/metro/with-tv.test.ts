import { mkdirSync, mkdtempSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { z } from 'zod';
import { PACKAGE_ROOT } from '../package-paths.ts';
import * as metro from './index.ts';
import { OWN_PACKAGE, withTV } from './with-tv.ts';
import { withWebOS } from './with-webos.ts';

const example = fileURLToPath(new URL('../../example/', import.meta.url));
const manifest = z
  .object({
    name: z.string(),
    exports: z.record(z.string(), z.unknown()),
    reactNativeTvPlatform: z.object({ metro: z.string() }),
  })
  .parse(JSON.parse(readFileSync(join(PACKAGE_ROOT, 'package.json'), 'utf8')));

let root: string;

function write(file: string, content: unknown): void {
  const path = join(root, file);
  mkdirSync(dirname(path), { recursive: true });
  writeFileSync(path, typeof content === 'string' ? content : JSON.stringify(content));
}

function platformPackage(name: string, platform: string): void {
  write(`node_modules/${name}/package.json`, {
    name,
    exports: { './metro': './metro.cjs', './package.json': './package.json' },
    reactNativeTvPlatform: { metro: './metro' },
  });
  write(
    `node_modules/${name}/metro.cjs`,
    `exports.withTvPlatform = (config) => ({ ...config, resolver: { ...config.resolver, platforms: [...config.resolver.platforms, '${platform}'] } });`,
  );
}

beforeEach(() => {
  root = mkdtempSync(join(tmpdir(), 'rn-webos-with-tv-'));
  write('node_modules/react-native/package.json', { name: 'react-native' });
});

afterEach(() => {
  rmSync(root, { recursive: true, force: true });
});

describe('withTV', () => {
  it('adds webos and the platform of every other TV package the app depends on, in the order of their names', () => {
    write('package.json', {
      dependencies: {
        'react-native': '1.0.0',
        'zz-tv': '1.0.0',
        [OWN_PACKAGE]: '1.0.0',
        '@kromatv/react-native-tizen': '1.0.0',
      },
    });
    platformPackage('zz-tv', 'zz');
    platformPackage('@kromatv/react-native-tizen', 'tizen');
    platformPackage(OWN_PACKAGE, 'a second copy of this package');

    const config = withTV({ projectRoot: root, resolver: { platforms: ['ios', 'android'] } });

    expect(config.resolver?.platforms).toEqual(['ios', 'android', 'tizen', 'webos', 'zz']);
  });

  it('is withWebOS for an app with no other TV platform', () => {
    const config = { projectRoot: example, resolver: { platforms: ['ios'] } };

    expect(withTV(config).resolver?.platforms).toEqual(withWebOS(config).resolver?.platforms);
    expect(withTV(config).resolver?.platforms).toEqual(['ios', 'webos']);
  });

  it('fails on a platform package whose Metro entry has no wrapper', () => {
    write('package.json', { dependencies: { 'react-native': '1.0.0', 'zz-tv': '1.0.0' } });
    platformPackage('zz-tv', 'zz');
    write('node_modules/zz-tv/metro.cjs', 'module.exports = {};');

    expect(() => withTV({ projectRoot: root })).toThrow('zz-tv declares a TV platform');
  });
});

describe('the package, as a TV platform', () => {
  it('is found by withTV from another package: its name, its declaration and the wrapper its Metro entry exports', () => {
    expect(manifest.name).toBe(OWN_PACKAGE);
    expect(manifest.exports).toHaveProperty(manifest.reactNativeTvPlatform.metro);
    expect(metro.withTvPlatform).toBe(withWebOS);
    expect(metro.withTV).toBe(withTV);
  });
});
