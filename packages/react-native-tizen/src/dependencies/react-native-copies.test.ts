import { mkdirSync, mkdtempSync, rmSync, symlinkSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, describe, expect, it } from 'vitest';
import { FixableError } from '../errors.ts';
import { reactNativeCopies, singleReactNativeIssue } from './react-native-copies.ts';

const dirs: string[] = [];

afterEach(() => {
  for (const dir of dirs.splice(0)) rmSync(dir, { recursive: true, force: true });
});

function write(dir: string, manifest: Record<string, unknown>): string {
  mkdirSync(dir, { recursive: true });
  writeFileSync(join(dir, 'package.json'), JSON.stringify(manifest));
  return dir;
}

const TVOS = { name: 'react-native-tvos', version: '0.88.0-0' };

function app(dependencies: Record<string, string>): string {
  const dir = mkdtempSync(join(tmpdir(), 'rn-tizen-copies-'));
  dirs.push(dir);
  return write(dir, { name: 'tv-app', dependencies });
}

describe('reactNativeCopies', () => {
  it("finds the upstream copy npm nests for a peer react-native-tvos's version does not satisfy", () => {
    const root = app({ 'react-native': 'npm:react-native-tvos@0.88.0-0' });
    const tvos = write(join(root, 'node_modules', 'react-native'), {
      ...TVOS,
      dependencies: { '@react-native-tvos/virtualized-lists': '0.88.0-0' },
    });
    write(join(tvos, 'node_modules', '@react-native-tvos', 'virtualized-lists'), {
      name: '@react-native-tvos/virtualized-lists',
      peerDependencies: { 'react-native': '0.88.0', 'react-native-web': '*' },
    });
    write(join(tvos, 'node_modules', 'react-native'), { name: 'react-native', version: '0.88.0' });

    const copies = reactNativeCopies(root);
    const issue = singleReactNativeIssue(root);

    expect(copies.map(({ name, version, dependents }) => ({ name, version, dependents }))).toEqual([
      { ...TVOS, dependents: ['tv-app'] },
      {
        name: 'react-native',
        version: '0.88.0',
        dependents: ['@react-native-tvos/virtualized-lists'],
      },
    ]);
    expect(issue).toBeInstanceOf(FixableError);
    expect(issue?.message).toBe(
      'The app loads 2 copies of react-native: react-native-tvos 0.88.0-0 at node_modules/react-native, for tv-app; react-native 0.88.0 at node_modules/react-native/node_modules/react-native, for @react-native-tvos/virtualized-lists',
    );
    expect(issue?.hint).toContain('npx rn-tizen init');
  });

  it("counts a linked package once, by where it really is, as pnpm's store lays them out", () => {
    const root = app({ 'react-native': 'npm:react-native-tvos@0.88.0-0', lib: '1.0.0' });
    const store = join(root, 'node_modules', '.pnpm');
    const tvos = write(
      join(store, 'react-native-tvos@0.88.0-0', 'node_modules', 'react-native-tvos'),
      TVOS,
    );
    const lib = write(join(store, 'lib@1.0.0', 'node_modules', 'lib'), {
      name: 'lib',
      peerDependencies: { 'react-native': '*' },
    });
    symlinkSync(tvos, join(root, 'node_modules', 'react-native'), 'dir');
    symlinkSync(tvos, join(store, 'lib@1.0.0', 'node_modules', 'react-native'), 'dir');
    symlinkSync(lib, join(root, 'node_modules', 'lib'), 'dir');

    expect(reactNativeCopies(root)).toEqual([
      expect.objectContaining({ ...TVOS, dependents: ['lib', 'tv-app'] }),
    ]);
    expect(singleReactNativeIssue(root)).toBeNull();
  });

  it('names a few of the packages that load a copy, and skips what is missing or unreadable', () => {
    const root = app({ a: '1', b: '1', c: '1', d: '1', missing: '1', broken: '1' });
    for (const name of ['a', 'b', 'c', 'd']) {
      write(join(root, 'node_modules', name), { name, dependencies: { 'react-native': '0.88.0' } });
    }
    write(join(root, 'node_modules', 'a', 'node_modules', 'react-native'), {
      name: 'react-native',
      version: '0.88.0',
    });
    write(join(root, 'node_modules', 'react-native'), TVOS);
    mkdirSync(join(root, 'node_modules', 'broken'));
    writeFileSync(join(root, 'node_modules', 'broken', 'package.json'), '{ not json');

    expect(singleReactNativeIssue(root)?.message).toContain(
      'react-native-tvos 0.88.0-0 at node_modules/react-native, for b, c, d; react-native 0.88.0 at node_modules/a/node_modules/react-native, for a',
    );
  });

  it('lists how many more packages load a copy than it names', () => {
    const names = ['a', 'b', 'c', 'd', 'e'];
    const root = app(Object.fromEntries(names.map((name) => [name, '1'])));
    for (const name of names) {
      write(join(root, 'node_modules', name), { name, peerDependencies: { 'react-native': '*' } });
    }
    write(join(root, 'node_modules', 'react-native'), {});
    write(join(root, 'node_modules', 'e', 'node_modules', 'react-native'), TVOS);

    expect(singleReactNativeIssue(root)?.message).toContain(
      'react-native unknown at node_modules/react-native, for a, b, c and 1 more',
    );
  });
});
