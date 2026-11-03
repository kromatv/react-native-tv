import { mkdirSync, mkdtempSync, readdirSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import type { AppProject } from '../app/app-project.ts';
import { resolveTizenConfig } from '../config/tizen-config.ts';
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
  const installed = { name: 'installed', version: '1.0.0', dir: root };
  const app: AppProject = {
    root,
    config: resolveTizenConfig({ name: 'Demo', tizen: { package: 'org.example.demo', ...tizen } }),
    entryFile: join(root, 'index.ts'),
    packages: {
      reactNative: installed,
      expo: installed,
      expoModulesCore: installed,
      expoModulesJsi: installed,
    },
    tizenDir: join(root, 'tizen'),
  };
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
});
