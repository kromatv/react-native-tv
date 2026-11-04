import { mkdirSync, mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, describe, expect, it } from 'vitest';
import { findPackageManager, packageRunner } from './package-manager.ts';

const dirs: string[] = [];

afterEach(() => {
  for (const dir of dirs.splice(0)) rmSync(dir, { recursive: true, force: true });
});

function project(lockfile?: string): string {
  const dir = mkdtempSync(join(tmpdir(), 'rn-tizen-runner-'));
  dirs.push(dir);
  writeFileSync(join(dir, 'package.json'), '{}');
  if (lockfile !== undefined) writeFileSync(join(dir, lockfile), '');
  return dir;
}

describe('packageRunner', () => {
  it.each([
    ['bun.lock', 'bunx'],
    ['bun.lockb', 'bunx'],
    ['pnpm-lock.yaml', 'pnpm exec'],
    ['yarn.lock', 'yarn'],
    ['package-lock.json', 'npx'],
  ])('reads %s as %s', (lockfile, runner) => {
    expect(packageRunner(project(lockfile))).toBe(runner);
  });

  it("takes the workspace root's lockfile for an app inside it, and npx without one", () => {
    const workspace = project('bun.lock');
    const app = join(workspace, 'apps', 'tv');
    mkdirSync(app, { recursive: true });

    expect(packageRunner(app)).toBe('bunx');
    expect(packageRunner(project())).toBe('npx');
  });
});

describe('findPackageManager', () => {
  it('names the root it installs from: the workspace root, or the app without a lockfile', () => {
    const workspace = project('pnpm-lock.yaml');
    const app = join(workspace, 'apps', 'tv');
    mkdirSync(app, { recursive: true });
    const alone = project();

    expect(findPackageManager(app)).toEqual({ name: 'pnpm', root: workspace });
    expect(findPackageManager(alone)).toEqual({ name: 'npm', root: alone });
  });
});
