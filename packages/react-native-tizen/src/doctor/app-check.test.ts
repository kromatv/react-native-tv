import { mkdtempSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, describe, expect, it, vi } from 'vitest';
import { appDir } from '../app/app-dir.fixtures.ts';
import { fakeApp } from '../app/app.fixtures.ts';
import * as appProject from '../app/app-project.ts';
import { openBuild } from '../commands/build-options.ts';
import { EXPO_TIZEN_DEPRECATION } from '../config/tizen-settings.ts';
import { checkApp } from './app-check.ts';

const dirs: string[] = [];

afterEach(() => {
  vi.restoreAllMocks();
  vi.unstubAllEnvs();
  for (const dir of dirs.splice(0)) rmSync(dir, { recursive: true, force: true });
});

function legacyApp(): string {
  const dir = appDir({
    name: 'Legacy',
    version: '1.2.0',
    tizen: { package: 'org.example.legacy' },
  });
  dirs.push(dir);
  return dir;
}

describe('an app configured under expo.tizen', () => {
  it('warns in the doctor', async () => {
    expect(await checkApp(legacyApp())).toMatchObject({
      status: 'warn',
      detail: 'org.example.legacy 1.2.0',
      fix: EXPO_TIZEN_DEPRECATION,
    });
  });

  it('warns before a build', () => {
    let printed = '';
    vi.spyOn(process.stdout, 'write').mockImplementation((chunk) => {
      printed += String(chunk);
      return true;
    });

    const dir = legacyApp();
    vi.stubEnv('TIZEN_SDK_DIR', dir);

    expect(() => openBuild({}, { cwd: dir })).toThrow('Tizen Studio not found');
    expect(printed).toContain(EXPO_TIZEN_DEPRECATION);
  });
});

describe('checkApp', () => {
  it('skips a directory with no Expo config', async () => {
    const dir = mkdtempSync(join(tmpdir(), 'rn-tizen-no-app-'));
    dirs.push(dir);

    expect(await checkApp(dir)).toMatchObject({
      status: 'skip',
      detail: 'no Expo app here (pass --project)',
    });
  });

  it('passes with the package id, version and react-native of an app configured through the plugin', async () => {
    const dir = legacyApp();
    const app = fakeApp(dir);
    vi.spyOn(appProject, 'loadAppProject').mockReturnValue({
      ...app,
      config: { ...app.config, deprecation: null },
    });

    expect(await checkApp(dir)).toEqual({
      title: 'App',
      status: 'ok',
      detail: 'org.example.demo 1.0.0, react-native 0.88.0',
    });
  });
});
