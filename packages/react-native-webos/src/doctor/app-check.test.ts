import { mkdirSync, mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, describe, expect, it, vi } from 'vitest';
import { appDir } from '../app/app-dir.fixtures.ts';
import { fakeApp } from '../app/app.fixtures.ts';
import * as appProject from '../app/app-project.ts';
import { checkApp } from './app-check.ts';

const dirs: string[] = [];

afterEach(() => {
  vi.restoreAllMocks();
  vi.unstubAllEnvs();
  for (const dir of dirs.splice(0)) rmSync(dir, { recursive: true, force: true });
});

function configuredApp(): string {
  const dir = appDir({
    name: 'Configured',
    version: '1.2.0',
    plugins: [['@kromatv/react-native-webos', { package: 'org.example.configured' }]],
  });
  dirs.push(dir);
  return dir;
}

describe('checkApp', () => {
  it('skips a directory with no Expo config', async () => {
    const dir = mkdtempSync(join(tmpdir(), 'rn-webos-no-app-'));
    dirs.push(dir);

    expect(await checkApp(dir)).toMatchObject({
      status: 'skip',
      detail: 'no Expo app here (pass --project)',
    });
  });

  it('passes with the app id, version and react-native of an app configured through the plugin', async () => {
    expect(await checkApp(configuredApp())).toEqual({
      title: 'App',
      status: 'ok',
      detail: expect.stringMatching(/^org\.example\.configured 1\.2\.0, react-native-tvos /),
    });
  });

  it('fails with what is wrong in the config, and where to fix it', async () => {
    const dir = appDir({ name: 'Unconfigured' });
    dirs.push(dir);

    expect(await checkApp(dir)).toMatchObject({
      status: 'fail',
      detail: expect.stringContaining('The app has no webOS settings'),
    });
  });
});

describe('an app that loads two react-natives', () => {
  it('fails the doctor with the copies and the fix', async () => {
    const dir = mkdtempSync(join(tmpdir(), 'rn-webos-two-copies-'));
    dirs.push(dir);
    const install = (path: string, manifest: Record<string, unknown>) => {
      mkdirSync(join(dir, path), { recursive: true });
      writeFileSync(join(dir, path, 'package.json'), JSON.stringify(manifest));
    };
    install('.', { name: 'tv-app', dependencies: { 'react-native': '*', lib: '*' } });
    install('node_modules/react-native', { name: 'react-native-tvos', version: '0.88.0-0' });
    install('node_modules/lib', { name: 'lib', dependencies: { 'react-native': '*' } });
    install('node_modules/lib/node_modules/react-native', {
      name: 'react-native',
      version: '0.88.0',
    });
    writeFileSync(join(dir, 'app.json'), '{}');
    vi.spyOn(appProject, 'loadAppProject').mockReturnValue(fakeApp(dir));

    expect(await checkApp(dir)).toMatchObject({
      status: 'fail',
      detail: expect.stringContaining('The app loads 2 copies of react-native'),
      fix: expect.stringContaining('rn-webos init'),
    });
  });
});
