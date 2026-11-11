import { mkdirSync, mkdtempSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, describe, expect, it, vi } from 'vitest';
import { runSync } from '../exec.ts';
import { pnpmMajor, setUpReactNative } from './set-up-react-native.ts';

vi.mock('../exec.ts', () => ({ runSync: vi.fn() }));

const TVOS = 'npm:react-native-tvos@0.88.0-0';
const dirs: string[] = [];

afterEach(() => {
  vi.mocked(runSync).mockReset();
  for (const dir of dirs.splice(0)) rmSync(dir, { recursive: true, force: true });
});

function project(manifest: Record<string, unknown>, lockfile?: string): string {
  const dir = mkdtempSync(join(tmpdir(), 'rn-tizen-deps-'));
  dirs.push(dir);
  writeFileSync(join(dir, 'package.json'), JSON.stringify(manifest, null, '\t'));
  if (lockfile !== undefined)
    writeFileSync(join(dir, lockfile), lockfile.endsWith('.json') ? '{}' : '');
  return dir;
}

const manifestOf = (dir: string) => JSON.parse(readFileSync(join(dir, 'package.json'), 'utf8'));
const upstream = { name: 'tv-app', dependencies: { expo: '~58.0.16', 'react-native': '0.88.1' } };
const noPnpm = () => {
  throw new Error("pnpm is not this app's package manager");
};

describe('setUpReactNative', () => {
  it.each([
    ['package-lock.json', 'overrides'],
    ['bun.lock', 'overrides'],
  ])("with %s, aliases react-native and overrides it with the app's own spec", (lockfile, key) => {
    const dir = project(upstream, lockfile);

    const setup = setUpReactNative(dir, noPnpm);

    expect(setup.alias).toEqual({ kind: 'set', spec: TVOS, file: join(dir, 'package.json') });
    expect(setup.override.kind).toBe('set');
    expect(manifestOf(dir)).toEqual({
      ...upstream,
      dependencies: { ...upstream.dependencies, 'react-native': TVOS },
      [key]: { 'react-native': '$react-native' },
    });
    expect(readFileSync(join(dir, 'package.json'), 'utf8')).toContain('\n\t"dependencies"');
  });

  it('gives Yarn the spec itself, and changes nothing the second time', () => {
    const dir = project(upstream, 'yarn.lock');

    setUpReactNative(dir, noPnpm);
    const again = setUpReactNative(dir, noPnpm);

    expect(manifestOf(dir).resolutions).toEqual({ 'react-native': TVOS });
    expect(again.alias.kind).toBe('present');
    expect(again.override.kind).toBe('present');
    expect(again.unlocked).toBe(false);
  });

  it('keeps a react-native-tvos the app already names, and overrides with that one', () => {
    const chosen = 'npm:react-native-tvos@0.88.1-0';
    const dir = project({ dependencies: { 'react-native': chosen } }, 'yarn.lock');

    const setup = setUpReactNative(dir, noPnpm);

    expect(setup.alias).toMatchObject({ kind: 'present', spec: chosen });
    expect(manifestOf(dir).resolutions).toEqual({ 'react-native': chosen });
  });

  it('writes pnpm 10 and older an override in package.json, and later ones in pnpm-workspace.yaml', () => {
    const old = project(upstream, 'pnpm-lock.yaml');
    const current = project(upstream, 'pnpm-lock.yaml');

    setUpReactNative(old, () => 10);
    const setup = setUpReactNative(current, () => 11);

    expect(manifestOf(old).pnpm).toEqual({ overrides: { 'react-native': '$react-native' } });
    expect(setup.override).toEqual({ kind: 'set', file: join(current, 'pnpm-workspace.yaml') });
    expect(manifestOf(current).pnpm).toBeUndefined();
  });

  it("overrides at a workspace's root with the spec, which the root does not depend on", () => {
    const root = project({ name: 'monorepo', private: true, workspaces: ['apps/*'] }, 'bun.lock');
    const app = join(root, 'apps', 'tv');
    mkdirSync(app, { recursive: true });
    writeFileSync(join(app, 'package.json'), JSON.stringify(upstream));

    const setup = setUpReactNative(app, noPnpm);

    expect(setup.manager).toEqual({ name: 'bun', root });
    expect(manifestOf(root).overrides).toEqual({ 'react-native': TVOS });
    expect(manifestOf(app).dependencies['react-native']).toBe(TVOS);
  });

  it("unlocks npm's upstream copy so that the install resolves it through the override", () => {
    const dir = project(upstream);
    writeFileSync(
      join(dir, 'package-lock.json'),
      JSON.stringify({ packages: { 'node_modules/react-native': { version: '0.88.1' } } }),
    );

    expect(setUpReactNative(dir, noPnpm).unlocked).toBe(true);
  });

  it('refuses a package.json that is not a JSON object', () => {
    const dir = project({});
    writeFileSync(join(dir, 'package.json'), '[]');

    expect(() => setUpReactNative(dir, noPnpm)).toThrow('is not a JSON object');
  });
});

describe('pnpmMajor', () => {
  it("reads pnpm's major version, and takes the current layout when it prints none", () => {
    vi.mocked(runSync).mockReturnValueOnce('10.34.9\n').mockReturnValueOnce('pnpm\n');

    expect(pnpmMajor('/app')).toBe(10);
    expect(pnpmMajor('/app')).toBe(11);
    expect(runSync).toHaveBeenCalledWith(['pnpm', '--version'], { cwd: '/app', timeoutMs: 30_000 });
  });
});
