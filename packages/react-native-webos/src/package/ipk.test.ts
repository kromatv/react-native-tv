import { execFileSync } from 'node:child_process';
import { chmodSync, mkdtempSync, readFileSync, rmSync, symlinkSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { basename, join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { findProgram } from '../doctor/find-program.ts';
import { FixableError } from '../errors.ts';
import { writeIpk } from './ipk.ts';
import { elfHeader, writeProbeApp } from './ipk.fixtures.ts';
import { readIpk, type Ipk } from './read-ipk.ts';

const NOW = new Date('2026-11-11T10:00:00Z');
const MTIME = NOW.getTime() / 1000;
const APP = 'usr/palm/applications/com.example.probe';
const ARES_PACKAGE = findProgram('ares-package');

let dir: string;
let appDir: string;

beforeEach(() => {
  dir = mkdtempSync(join(tmpdir(), 'rn-webos-ipk-'));
  appDir = join(dir, 'app');
  writeProbeApp(appDir);
});

afterEach(() => {
  rmSync(dir, { recursive: true, force: true });
});

async function packaged(): Promise<Ipk> {
  const { file } = await writeIpk({ appDir, outDir: join(dir, 'out'), now: NOW });
  return readIpk(readFileSync(file));
}

const shape = (ipk: Ipk) =>
  ipk.dataEntries
    .map((entry) => `${entry.path} ${entry.type} ${entry.mode.toString(8)}`)
    .sort((left, right) => (left < right ? -1 : 1));

describe('writeIpk', () => {
  it('names the package after the app, its version and the CPU', async () => {
    const written = await writeIpk({ appDir, outDir: join(dir, 'out'), now: NOW });

    expect(written).toEqual({
      file: join(dir, 'out', 'com.example.probe_1.2.3_arm.ipk'),
      appId: 'com.example.probe',
      version: '1.2.3',
      installedSize: written.installedSize,
    });
    expect(readFileSync(written.file).toString('latin1', 0, 8)).toBe('!<arch>\n');
  });

  it('holds three ar members with bare names, in the order the TV expects', async () => {
    const { members } = await packaged();

    expect(
      members.map(({ name, mtime, uid, gid, mode }) => ({ name, mtime, uid, gid, mode })),
    ).toEqual(
      ['debian-binary', 'control.tar.gz', 'data.tar.gz'].map((name) => ({
        name,
        mtime: MTIME,
        uid: 0,
        gid: 0,
        mode: '100644',
      })),
    );
    expect(members[0]?.data.toString()).toBe('2.0\n');
    expect([...(members[1]?.data.subarray(0, 3) ?? [])]).toEqual([0x1f, 0x8b, 0x08]);
  });

  it('writes the ten control lines, with the installed size in bytes', async () => {
    const { controlEntries, dataEntries } = await packaged();
    const files = dataEntries.filter((entry) => entry.type === 'file');
    const size = files.reduce((total, entry) => total + entry.data.byteLength, 0);

    expect(controlEntries.map((entry) => [entry.path, entry.type, entry.mode])).toEqual([
      ['control', 'file', 0o644],
    ]);
    expect(Buffer.from(controlEntries[0]?.data ?? []).toString()).toBe(
      [
        'Package: com.example.probe',
        'Version: 1.2.3',
        'Section: misc',
        'Priority: optional',
        'Architecture: arm',
        `Installed-Size: ${size}`,
        'Maintainer: N/A <nobody@example.com>',
        'Description: This is a webOS application.',
        'webOS-Package-Format-Version: 2',
        'webOS-Packager-Version: x.y.x',
        '',
      ].join('\n'),
    );
  });

  it('lays the app out under usr/palm, every directory first and world-writable', async () => {
    expect(shape(await packaged())).toEqual([
      'usr/ directory 777',
      'usr/palm/ directory 777',
      'usr/palm/applications/ directory 777',
      `${APP}/ directory 777`,
      `${APP}/appdata/ directory 777`,
      `${APP}/appinfo.json file 644`,
      `${APP}/icon.png file 644`,
      `${APP}/react-native-webos file 755`,
      `${APP}/res/ directory 777`,
      `${APP}/res/assets/ directory 777`,
      `${APP}/res/assets/a.png file 644`,
      `${APP}/res/index.webos.hbc file 644`,
      'usr/palm/packages/ directory 777',
      'usr/palm/packages/com.example.probe/ directory 777',
      'usr/palm/packages/com.example.probe/packageinfo.json file 644',
    ]);
  });

  it('sorts the entries, dates them all with the time of packaging and owns them by root', async () => {
    const { dataEntries } = await packaged();
    const paths = dataEntries.map((entry) => entry.path);

    expect(paths).toEqual([...paths].sort((left, right) => (left < right ? -1 : 1)));
    expect(new Set(dataEntries.map((entry) => `${entry.mtime} ${entry.uid} ${entry.gid}`))).toEqual(
      new Set([`${MTIME} 0 0`]),
    );
  });

  it('copies every file byte for byte and writes packageinfo.json', async () => {
    const { dataEntries } = await packaged();
    const bytes = (path: string) =>
      Buffer.from(dataEntries.find((entry) => entry.path === path)?.data ?? []);

    expect(bytes(`${APP}/res/index.webos.hbc`).toString()).toBe('bytecode');
    expect(bytes(`${APP}/react-native-webos`).equals(elfHeader(40))).toBe(true);
    expect(bytes('usr/palm/packages/com.example.probe/packageinfo.json').toString()).toBe(
      `${JSON.stringify({ id: 'com.example.probe', version: '1.2.3', app: 'com.example.probe' }, null, 2)}\n`,
    );
  });

  it('gives the executable its mode whatever the working tree says, and keeps an executable file executable', async () => {
    chmodSync(join(appDir, 'react-native-webos'), 0o600);
    chmodSync(join(appDir, 'icon.png'), 0o700);

    const modes = new Map(
      shape(await packaged()).map((line) => [line.split(' ')[0], line.split(' ')[2]]),
    );

    expect(modes.get(`${APP}/react-native-webos`)).toBe('755');
    expect(modes.get(`${APP}/icon.png`)).toBe('755');
  });

  it('differs between two builds of one tree by the timestamps only', async () => {
    const first = await writeIpk({ appDir, outDir: join(dir, 'one'), now: NOW });
    const second = await writeIpk({ appDir, outDir: join(dir, 'two'), now: NOW });
    const later = await writeIpk({
      appDir,
      outDir: join(dir, 'three'),
      now: new Date(NOW.getTime() + 1000),
    });

    expect(readFileSync(second.file).equals(readFileSync(first.file))).toBe(true);
    expect(readFileSync(later.file).equals(readFileSync(first.file))).toBe(false);
    expect(shape(readIpk(readFileSync(later.file)))).toEqual(
      shape(readIpk(readFileSync(first.file))),
    );
  });

  it('refuses an executable that is not 32-bit ARM, with what to build instead', async () => {
    writeFileSync(join(appDir, 'react-native-webos'), elfHeader(183));

    await expect(writeIpk({ appDir, outDir: dir })).rejects.toBeInstanceOf(FixableError);
    writeFileSync(join(appDir, 'react-native-webos'), '#!/bin/sh\n');
    await expect(writeIpk({ appDir, outDir: dir })).rejects.toThrow('not a 32-bit ARM executable');
  });

  it('refuses an appinfo.json LG would refuse, a missing executable and a symlink', async () => {
    symlinkSync('icon.png', join(appDir, 'link.png'));
    await expect(writeIpk({ appDir, outDir: dir })).rejects.toThrow(
      'neither a file nor a directory',
    );
    rmSync(join(appDir, 'link.png'));

    rmSync(join(appDir, 'react-native-webos'));
    await expect(writeIpk({ appDir, outDir: dir })).rejects.toThrow('holds no react-native-webos');

    writeProbeApp(join(dir, 'reserved'), { id: 'com.webos.app.mine' });
    await expect(writeIpk({ appDir: join(dir, 'reserved'), outDir: dir })).rejects.toThrow(
      'appinfo.json: id:',
    );
    writeProbeApp(join(dir, 'version'), { version: '1.0' });
    await expect(writeIpk({ appDir: join(dir, 'version'), outDir: dir })).rejects.toThrow(
      'appinfo.json: version:',
    );
    await expect(writeIpk({ appDir: join(dir, 'missing'), outDir: dir })).rejects.toThrow(
      'holds no appinfo.json',
    );
  });
});

describe.skipIf(ARES_PACKAGE === null)("against LG's ares-package", () => {
  function aresPackaged(): Ipk {
    const out = join(dir, 'ares');
    execFileSync(ARES_PACKAGE ?? '', ['--no-minify', appDir, '-o', out], { stdio: 'pipe' });
    return readIpk(readFileSync(join(out, 'com.example.probe_1.2.3_arm.ipk')));
  }

  it('writes the same ar members, the same file name and the same control fields but the size', async () => {
    const ares = aresPackaged();
    const { file } = await writeIpk({ appDir, outDir: join(dir, 'out'), now: NOW });
    const ours = readIpk(readFileSync(file));
    const header = (ipk: Ipk) =>
      ipk.members.map(({ name, uid, gid, mode, data }) => ({
        name,
        uid,
        gid,
        mode,
        gzip: data[0] === 0x1f,
      }));
    const fields = (ipk: Ipk) =>
      [...ipk.control]
        .filter(([name]) => name !== 'Installed-Size')
        .map((field) => field.join(': '));

    expect(basename(file)).toBe('com.example.probe_1.2.3_arm.ipk');
    expect(header(ours)).toEqual(header(ares));
    expect(fields(ours)).toEqual(fields(ares));
    expect([...ours.control.keys()]).toEqual([...ares.control.keys()]);
    expect(ours.controlEntries.map((entry) => [entry.path, entry.type])).toEqual(
      ares.controlEntries.map((entry) => [entry.path, entry.type]),
    );
  }, 30_000);

  it('installs the same tree with the same modes, packageinfo.json apart', async () => {
    const ares = aresPackaged();
    const ours = await packaged();
    const withoutPackageInfo = (ipk: Ipk) =>
      shape(ipk).filter((line) => !line.includes('packageinfo.json'));
    const contents = (ipk: Ipk) =>
      new Map(
        ipk.dataEntries
          .filter((entry) => entry.type === 'file')
          .map((entry) => [entry.path, Buffer.from(entry.data).toString('base64')]),
      );

    expect(withoutPackageInfo(ours)).toEqual(withoutPackageInfo(ares));
    expect(contents(ours)).toEqual(contents(ares));
    expect(shape(ares)).toContain('usr/palm/packages/com.example.probe/packageinfo.json file 666');
  }, 30_000);

  it('counts the installed files where ares also counts its directories', async () => {
    const ares = aresPackaged();
    const ours = await packaged();
    const files = ours.dataEntries.reduce((total, entry) => total + entry.data.byteLength, 0);

    expect(Number(ours.control.get('Installed-Size'))).toBe(files);
    expect(Number(ares.control.get('Installed-Size'))).toBeGreaterThanOrEqual(files);
  }, 30_000);
});
