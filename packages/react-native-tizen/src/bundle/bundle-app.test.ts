import { existsSync, mkdirSync, mkdtempSync, readdirSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { fakeApp } from '../app/app.fixtures.ts';
import { FixableError } from '../errors.ts';
import { recordingTask } from '../fakes.fixtures.ts';
import { bundleApp } from './bundle-app.ts';

vi.mock('./hermesc.ts', () => ({ locateHermesc: vi.fn(async () => '/opt/hermesc') }));

let root: string;

function app(tizen: Record<string, unknown> = {}) {
  const expo = join(root, 'expo');
  mkdirSync(join(expo, 'bin'), { recursive: true });
  writeFileSync(join(expo, 'package.json'), '{"name":"expo","version":"58.0.0"}');
  writeFileSync(join(expo, 'bin', 'cli'), '');
  const project = fakeApp(root, tizen);
  return {
    ...project,
    packages: { ...project.packages, expo: { ...project.packages.expo, dir: expo } },
  };
}

beforeEach(() => {
  root = mkdtempSync(join(tmpdir(), 'rn-tizen-bundle-'));
});

afterEach(() => {
  rmSync(root, { recursive: true, force: true });
});

describe('bundleApp', () => {
  it("bundles with Expo's export:embed for tizen, then compiles the bundle with hermesc", async () => {
    const task = recordingTask();
    const out = join(root, 'out');

    const bundle = await bundleApp(task, app({ locales: ['fr'] }), out);

    expect(bundle).toEqual({
      javascript: join(out, 'index.tizen.js'),
      bytecode: join(out, 'index.tizen.hbc'),
      assetsDir: join(out, 'assets'),
    });
    expect(task.commands[0]).toEqual(
      expect.arrayContaining(['export:embed', '--platform', 'tizen', '--assets-dest', out]),
    );
    expect(task.options[0]?.env?.RN_TIZEN_INTL_LOCALES).toBe('fr');
    expect(task.commands[1]?.[0]).toBe('/opt/hermesc');
    expect(task.progressed).toEqual(['Metro', 'Hermes']);
  });

  it('bundles again into a directory it bundled into before', async () => {
    const out = join(root, 'out');
    await bundleApp(recordingTask(), app(), out);
    writeFileSync(join(out, 'index.tizen.js'), '');

    await expect(bundleApp(recordingTask(), app(), out)).resolves.toBeDefined();
  });

  it('refuses an --out holding files of another origin, and leaves its assets/ alone', async () => {
    mkdirSync(join(root, 'assets'));
    writeFileSync(join(root, 'assets', 'icon.png'), 'png');

    const bundling = bundleApp(recordingTask(), app(), root);

    await expect(bundling).rejects.toBeInstanceOf(FixableError);
    await expect(bundling).rejects.toThrow('holds files rn-tizen did not write');
    expect(readdirSync(join(root, 'assets'))).toEqual(['icon.png']);
  });

  it('names the locales FormatJS has no data for, before it bundles', async () => {
    const task = recordingTask();

    await expect(bundleApp(task, app({ locales: ['xx-Nope'] }), join(root, 'out'))).rejects.toThrow(
      'no Intl data for xx-Nope',
    );
    expect(task.commands).toEqual([]);
    expect(existsSync(join(root, 'out'))).toBe(false);
  });
});
