import { rmSync } from 'node:fs';
import { afterEach, describe, expect, it, vi } from 'vitest';
import { appDir } from '../app/app-dir.fixtures.ts';
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
