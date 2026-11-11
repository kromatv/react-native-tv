import {
  existsSync,
  mkdirSync,
  mkdtempSync,
  readdirSync,
  readFileSync,
  rmSync,
  statSync,
  writeFileSync,
} from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { fakeApp } from '../app/app.fixtures.ts';
import { RUNTIME_PERMISSIONS } from '../config/webos-config.ts';
import { writeAppConfig } from './app-config-file.ts';
import { elfHeader } from './ipk.fixtures.ts';
import { writeIpk } from './ipk.ts';
import { plainPng } from './png.fixtures.ts';
import { readIpk } from './read-ipk.ts';
import { stageAppDir, stagedExecutable } from './stage-app.ts';

vi.mock('./app-config-file.ts', () => ({ writeAppConfig: vi.fn(async () => {}) }));

let root: string;

function file(name: string, contents: string | Uint8Array = name): string {
  const path = join(root, name);
  writeFileSync(path, contents);
  return path;
}

function inputs(
  webos: Record<string, unknown> = {},
  log: { host: string; port: number } | null = null,
) {
  const assetsDir = join(root, 'bundle-assets');
  mkdirSync(join(assetsDir, 'images'), { recursive: true });
  writeFileSync(join(assetsDir, 'images', 'logo.png'), 'logo');
  return {
    app: fakeApp(root, webos),
    runtime: file('built-runtime', elfHeader(40)),
    bundle: { javascript: file('index.js'), bytecode: file('index.hbc'), assetsDir },
    log,
    icuData: file('icudt78l.dat'),
    caBundle: file('cacert-2026-11-05.pem', 'certificates'),
  };
}

const json = (path: string): unknown => JSON.parse(readFileSync(path, 'utf8'));

beforeEach(() => {
  root = mkdtempSync(join(tmpdir(), 'rn-webos-stage-'));
});

afterEach(() => {
  rmSync(root, { recursive: true, force: true });
  vi.clearAllMocks();
});

describe('stageAppDir', () => {
  it('lays out the app directory a package installs', async () => {
    const dir = await stageAppDir(inputs());

    expect(dir).toBe(join(root, 'webos', 'app'));
    expect(readdirSync(dir).sort()).toEqual([
      'appdata',
      'appinfo.json',
      'icon.png',
      'largeIcon.png',
      'react-native-webos',
      'res',
    ]);
    expect(readdirSync(join(dir, 'appdata'))).toEqual([]);
    expect(readdirSync(join(dir, 'res')).sort()).toEqual([
      'THIRD_PARTY_NOTICES.md',
      'assets',
      'cacert.pem',
      'icudt78l.dat',
      'index.webos.hbc',
      'react-native-webos.json',
    ]);
    expect(readFileSync(join(dir, 'res', 'assets', 'images', 'logo.png'), 'utf8')).toBe('logo');
    expect(readFileSync(join(dir, 'res', 'cacert.pem'), 'utf8')).toBe('certificates');
    expect(writeAppConfig).toHaveBeenCalledWith(root, join(dir, 'res'));
  });

  it('copies the executable, executable, where a crash is symbolized from', async () => {
    const dir = await stageAppDir(inputs());
    const executable = stagedExecutable(join(root, 'webos'));

    expect(executable).toBe(join(dir, 'react-native-webos'));
    expect(readFileSync(executable).equals(elfHeader(40))).toBe(true);
    expect(statSync(executable).mode & 0o777).toBe(0o755);
  });

  it('writes the appinfo.json of a native app from the config', async () => {
    const dir = await stageAppDir(
      inputs({ title: 'Demo TV', version: '2.3.4', vendor: 'Example' }),
    );

    expect(json(join(dir, 'appinfo.json'))).toEqual({
      id: 'org.example.demo',
      version: '2.3.4',
      vendor: 'Example',
      type: 'native',
      main: 'react-native-webos',
      title: 'Demo TV',
      icon: 'icon.png',
      largeIcon: 'largeIcon.png',
      nativeLifeCycleInterfaceVersion: 2,
      handlesRelaunch: true,
      requiredACG: [...RUNTIME_PERMISSIONS],
    });
  });

  it('adds the splash image, the tile color and the ACG groups an app asks for', async () => {
    file('splash.png', plainPng(1920, 1080));

    const dir = await stageAppDir(
      inputs({ splash: './splash.png', iconColor: '#101010', permissions: ['network.query'] }),
    );

    expect(json(join(dir, 'appinfo.json'))).toMatchObject({
      splashBackground: 'splash.png',
      iconColor: '#101010',
      requiredACG: [...RUNTIME_PERMISSIONS, 'network.query'],
    });
    expect(existsSync(join(dir, 'splash.png'))).toBe(true);
  });

  it('declares the window transparent for an app that depends on expo-video, and for no other', async () => {
    const plain = await stageAppDir(inputs());
    expect(json(join(plain, 'appinfo.json'))).not.toHaveProperty('transparent');

    const video = join(root, 'node_modules', 'expo-video');
    mkdirSync(video, { recursive: true });
    writeFileSync(join(video, 'package.json'), '{"name":"expo-video","version":"58.0.12"}');
    const withVideo = await stageAppDir(inputs());

    expect(json(join(withVideo, 'appinfo.json'))).toMatchObject({ transparent: true });
  });

  it('writes the runtime config of a release build, and of a development build with its log endpoint', async () => {
    const release = await stageAppDir(inputs());
    expect(json(join(release, 'res', 'react-native-webos.json'))).toEqual({
      appKey: 'main',
      packageId: 'org.example.demo',
      bundleFile: 'index.webos.hbc',
      development: false,
    });

    const development = await stageAppDir(inputs({}, { host: '192.168.1.20', port: 8099 }));
    expect(json(join(development, 'res', 'react-native-webos.json'))).toMatchObject({
      development: true,
      log: { host: '192.168.1.20', port: 8099 },
    });
  });

  it('replaces the last app directory, and keeps webos/ out of git', async () => {
    const dir = await stageAppDir(inputs());
    writeFileSync(join(dir, 'res', 'assets', 'stale.png'), 'stale');

    await stageAppDir(inputs());

    expect(existsSync(join(dir, 'res', 'assets', 'stale.png'))).toBe(false);
    expect(readFileSync(join(root, 'webos', '.gitignore'), 'utf8')).toBe('*\n');
  });

  it('stages what writeIpk packages', async () => {
    const dir = await stageAppDir(inputs());

    const { file: ipk, appId } = await writeIpk({ appDir: dir, outDir: join(root, 'webos') });
    const paths = readIpk(readFileSync(ipk)).dataEntries.map((entry) => entry.path);

    expect(appId).toBe('org.example.demo');
    expect(paths).toEqual(
      expect.arrayContaining([
        'usr/palm/applications/org.example.demo/appdata/',
        'usr/palm/applications/org.example.demo/react-native-webos',
        'usr/palm/applications/org.example.demo/res/react-native-webos.json',
        'usr/palm/packages/org.example.demo/packageinfo.json',
      ]),
    );
  });
});
