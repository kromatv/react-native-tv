import { mkdirSync, mkdtempSync, readdirSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { fakeApp } from '../app/app.fixtures.ts';
import { platformProfile, SUPPORTED_PLATFORMS } from '../webos/platform-profiles.ts';
import { writeWebOSProject } from './webos-project.ts';

vi.mock('./app-config-file.ts', () => ({ writeAppConfig: vi.fn(async () => {}) }));

let root: string;

function file(name: string): string {
  const path = join(root, name);
  writeFileSync(path, name);
  return path;
}

function inputs(webos: Record<string, unknown>) {
  const app = fakeApp(root, webos);
  const assetsDir = join(root, 'assets');
  mkdirSync(assetsDir);
  return {
    app,
    profile: platformProfile(SUPPORTED_PLATFORMS[0] ?? ''),
    runtimeLibrary: file('libreact-native-webos.so'),
    bundle: { javascript: file('index.js'), bytecode: file('index.hbc'), assetsDir },
    log: null,
    icuData: file('icudt77l.dat'),
  };
}

beforeEach(() => {
  root = mkdtempSync(join(tmpdir(), 'rn-webos-project-'));
});

afterEach(() => {
  rmSync(root, { recursive: true, force: true });
});

describe('writeWebOSProject', () => {
  it("copies every C# source of the shell next to the app's manifest", async () => {
    const dir = await writeWebOSProject(inputs({}));

    const files = readdirSync(dir);
    expect(files).toEqual(
      expect.arrayContaining(['ReactNativeApplication.cs', 'webos-manifest.xml']),
    );
    expect(files).not.toContain('PreviewService');
  });

  it('adds the preview service app with its own sources for a personal preview', async () => {
    const dir = await writeWebOSProject(inputs({ preview: { refreshMinutes: 60 } }));

    expect(readdirSync(join(dir, 'PreviewService'))).toEqual(
      expect.arrayContaining([
        'PreviewRefresher.cs',
        'PreviewService.csproj',
        'webos-manifest.xml',
      ]),
    );
  });

  it('writes into a webos/ that ignores itself', async () => {
    const dir = await writeWebOSProject(inputs({}));

    expect(readFileSync(join(dir, '.gitignore'), 'utf8')).toBe('*\n');
  });

  it("leaves a webos/ of the app's own untouched", async () => {
    mkdirSync(join(root, 'webos'));
    writeFileSync(join(root, 'webos', 'config.xml'), '<widget />');

    await expect(writeWebOSProject(inputs({}))).rejects.toThrow("is the app's own");
    expect(readdirSync(join(root, 'webos'))).toEqual(['config.xml']);
  });

  it("packages the app's own PNG icon", async () => {
    writeFileSync(join(root, 'icon.png'), 'png bytes');

    const dir = await writeWebOSProject(inputs({ icon: './icon.png' }));

    expect(readFileSync(join(dir, 'shared', 'res', 'icon.png'), 'utf8')).toBe('png bytes');
  });

  it('refuses an icon that does not exist, or is not a PNG', async () => {
    writeFileSync(join(root, 'icon.jpg'), 'jpeg bytes');

    await expect(writeWebOSProject(inputs({ icon: './missing.png' }))).rejects.toThrow(
      'The app icon ./missing.png does not exist',
    );
    rmSync(join(root, 'assets'), { recursive: true });
    await expect(writeWebOSProject(inputs({ icon: './icon.jpg' }))).rejects.toThrow(
      'The app icon ./icon.jpg is not a PNG',
    );
  });
});
