import { mkdirSync, mkdtempSync, readdirSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { fakeApp } from '../app/app.fixtures.ts';
import { platformProfile, SUPPORTED_PLATFORMS } from '../tizen/platform-profiles.ts';
import { writeTizenProject } from './tizen-project.ts';

vi.mock('./app-config-file.ts', () => ({ writeAppConfig: vi.fn(async () => {}) }));

let root: string;

function file(name: string): string {
  const path = join(root, name);
  writeFileSync(path, name);
  return path;
}

function inputs(tizen: Record<string, unknown>) {
  const app = fakeApp(root, tizen);
  const assetsDir = join(root, 'assets');
  mkdirSync(assetsDir);
  return {
    app,
    profile: platformProfile(SUPPORTED_PLATFORMS[0] ?? ''),
    runtimeLibrary: file('libreact-native-tizen.so'),
    bundle: { javascript: file('index.js'), bytecode: file('index.hbc'), assetsDir },
    log: null,
    icuData: file('icudt77l.dat'),
  };
}

beforeEach(() => {
  root = mkdtempSync(join(tmpdir(), 'rn-tizen-project-'));
});

afterEach(() => {
  rmSync(root, { recursive: true, force: true });
});

describe('writeTizenProject', () => {
  it("copies every C# source of the shell next to the app's manifest", async () => {
    const dir = await writeTizenProject(inputs({}));

    const files = readdirSync(dir);
    expect(files).toEqual(
      expect.arrayContaining(['ReactNativeApplication.cs', 'tizen-manifest.xml']),
    );
    expect(files).not.toContain('PreviewService');
  });

  it('adds the preview service app with its own sources for a personal preview', async () => {
    const dir = await writeTizenProject(inputs({ preview: { refreshMinutes: 60 } }));

    expect(readdirSync(join(dir, 'PreviewService'))).toEqual(
      expect.arrayContaining([
        'PreviewRefresher.cs',
        'PreviewService.csproj',
        'tizen-manifest.xml',
      ]),
    );
  });

  it('writes into a tizen/ that ignores itself', async () => {
    const dir = await writeTizenProject(inputs({}));

    expect(readFileSync(join(dir, '.gitignore'), 'utf8')).toBe('*\n');
  });

  it("leaves a tizen/ of the app's own untouched", async () => {
    mkdirSync(join(root, 'tizen'));
    writeFileSync(join(root, 'tizen', 'config.xml'), '<widget />');

    await expect(writeTizenProject(inputs({}))).rejects.toThrow("is the app's own");
    expect(readdirSync(join(root, 'tizen'))).toEqual(['config.xml']);
  });
});
