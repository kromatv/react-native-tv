import { execFileSync } from 'node:child_process';
import { mkdtempSync, readFileSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { basename, join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { findProgram } from '../doctor/find-program.ts';
import { writeIpk } from './ipk.ts';
import { writeProbeApp } from './ipk.fixtures.ts';
import { readIpk, type Ipk } from './read-ipk.ts';

const NOW = new Date('2026-11-11T10:00:00Z');
const ARES_PACKAGE = findProgram('ares-package');

let dir: string;
let appDir: string;

beforeEach(() => {
  dir = mkdtempSync(join(tmpdir(), 'rn-webos-ares-'));
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
