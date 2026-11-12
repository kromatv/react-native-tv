import { mkdirSync, mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { dirname, join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { loadTvPlatform, tvPlatformPackages } from './tv-platforms.ts';

let root: string;

function write(file: string, content: unknown): void {
  const path = join(root, file);
  mkdirSync(dirname(path), { recursive: true });
  writeFileSync(path, typeof content === 'string' ? content : JSON.stringify(content));
}

function platformPackage(name: string, entry: string): void {
  write(`node_modules/${name}/package.json`, {
    name,
    exports: { './metro': './metro.cjs', './package.json': './package.json' },
    reactNativeTvPlatform: { metro: './metro' },
  });
  write(`node_modules/${name}/metro.cjs`, entry);
}

beforeEach(() => {
  root = mkdtempSync(join(tmpdir(), 'rn-webos-tv-platforms-'));
});

afterEach(() => {
  rmSync(root, { recursive: true, force: true });
});

describe('tvPlatformPackages', () => {
  it('finds the dependencies that declare a TV platform, by name, whatever kind of dependency', () => {
    write('package.json', {
      dependencies: { 'tv-b': '1.0.0', plain: '1.0.0', missing: '1.0.0' },
      devDependencies: { '@scope/tv-a': '1.0.0', 'tv-b': '1.0.0' },
    });
    platformPackage('tv-b', '');
    platformPackage('@scope/tv-a', '');
    platformPackage('not-a-dependency', '');
    write('node_modules/plain/package.json', { name: 'plain' });

    expect(tvPlatformPackages(root)).toEqual([
      { name: '@scope/tv-a', metro: '@scope/tv-a/metro' },
      { name: 'tv-b', metro: 'tv-b/metro' },
    ]);
  });

  it('skips a declaration that names no subpath, and a package that hides its package.json', () => {
    write('package.json', {
      dependencies: { absolute: '1.0.0', hidden: '1.0.0', broken: '1.0.0' },
    });
    write('node_modules/absolute/package.json', {
      name: 'absolute',
      reactNativeTvPlatform: { metro: '/etc/passwd' },
    });
    write('node_modules/hidden/package.json', {
      name: 'hidden',
      exports: { '.': './index.js' },
      reactNativeTvPlatform: { metro: './metro' },
    });
    write('node_modules/broken/package.json', '{');

    expect(tvPlatformPackages(root)).toEqual([]);
  });

  it('finds none for an app with no package.json, or one that is not an object', () => {
    expect(tvPlatformPackages(root)).toEqual([]);
    write('package.json', '[]');
    expect(tvPlatformPackages(root)).toEqual([]);
  });
});

describe('loadTvPlatform', () => {
  it("gives the wrapper the package's Metro entry exports", () => {
    write('package.json', { dependencies: { 'tv-b': '1.0.0' } });
    platformPackage('tv-b', 'exports.withTvPlatform = (config) => ({ ...config, tv: "b" });');
    const [platform] = tvPlatformPackages(root);

    expect(platform && loadTvPlatform(root, platform)({ projectRoot: root })).toEqual({
      projectRoot: root,
      tv: 'b',
    });
  });

  it('says which package declares a platform without a wrapper', () => {
    write('package.json', { dependencies: { 'tv-b': '1.0.0' } });
    platformPackage('tv-b', 'exports.withTvPlatform = "not a function";');
    const [platform] = tvPlatformPackages(root);

    expect(() => platform && loadTvPlatform(root, platform)).toThrow(
      'tv-b declares a TV platform, but tv-b/metro exports no withTvPlatform function',
    );
  });
});
