import { mkdirSync, mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import {
  acceptPush,
  capabilityPayload,
  fakeTv,
  TV_CAPABILITIES,
  type FakeTv,
  type ServiceHandler,
  type SyncRequest,
} from '../sdb/fake-tv.fixtures.ts';
import { writePkcs12 } from '../signing/pkcs12-write.ts';
import { testSigner } from '../signing/signing.fixtures.ts';
import { readZip } from '../signing/zip-read.ts';
import { writeZip } from '../signing/zip-write.ts';
import { installCommand } from './install.ts';
import { uninstallCommand } from './uninstall.ts';

const VARIABLES = ['TIZEN_SDK_DATA_DIR', 'XDG_STATE_HOME'];
let dir = '';
let saved: Record<string, string | undefined> = {};
const tvs: FakeTv[] = [];

function answer(text: string): ServiceHandler {
  return (stream) => {
    stream.write(text);
    stream.close();
  };
}

beforeEach(() => {
  dir = mkdtempSync(join(tmpdir(), 'rn-tizen-install-'));
  saved = Object.fromEntries(VARIABLES.map((name) => [name, process.env[name]]));
  process.env.TIZEN_SDK_DATA_DIR = join(dir, 'data');
  process.env.XDG_STATE_HOME = join(dir, 'state');
  vi.spyOn(process.stdout, 'write').mockImplementation(() => true);
  const author = testSigner('Author');
  const distributor = testSigner('Distributor');
  writeFileSync(join(dir, 'a.p12'), writePkcs12(author.key, author.chain, 'pw', 'a'));
  writeFileSync(join(dir, 'd.p12'), writePkcs12(distributor.key, distributor.chain, 'pw', 'd'));
  mkdirSync(join(dir, 'data', 'profile'), { recursive: true });
  writeFileSync(
    join(dir, 'data', 'profile', 'profiles.xml'),
    `<profiles active="TV" version="3.1"><profile name="TV">
<profileitem ca="" distributor="0" key="${join(dir, 'a.p12')}" password="pw" rootca=""/>
<profileitem ca="" distributor="1" key="${join(dir, 'd.p12')}" password="pw" rootca=""/>
</profile></profiles>`,
  );
  const config = '<widget><tizen:application id="AbCdE12345.App" package="AbCdE12345"/></widget>';
  writeFileSync(
    join(dir, 'app.wgt'),
    writeZip([{ name: 'config.xml', directory: false, data: Buffer.from(config) }]),
  );
});

afterEach(async () => {
  vi.restoreAllMocks();
  for (const [name, value] of Object.entries(saved)) {
    if (value === undefined) delete process.env[name];
    else process.env[name] = value;
  }
  await Promise.all(tvs.splice(0).map((tv) => tv.close()));
  rmSync(dir, { recursive: true, force: true });
});

async function tv(requests: SyncRequest[]): Promise<{ tv: FakeTv; pushed: () => Buffer }> {
  const started = await fakeTv({
    services: {
      'capability:': (stream) => {
        stream.write(capabilityPayload(TV_CAPABILITIES));
        stream.close();
      },
      'sync:': acceptPush(requests),
      'shell:0 vd_appinstall': answer('app_id[AbCdE12345.App] install completed\n'),
      'shell:0 execute': answer('... successfully launched pid = 7 with debug 0\r\n'),
      'shell:0 appuninstall': answer('app_id[AbCdE12345.App] uninstall completed\n'),
      'shell:0 getduid': answer('ABCDEFGH12345\r\n'),
    },
  });
  tvs.push(started);
  const pushed = () =>
    Buffer.concat(
      requests.filter((request) => request.id === 'DATA').map((request) => request.payload),
    );
  return { tv: started, pushed };
}

describe('rn-tizen install', () => {
  it('signs a widget with the active profile, installs it under its app id and launches it', async () => {
    const requests: SyncRequest[] = [];
    const { tv: started, pushed } = await tv(requests);

    await installCommand.run(['app.wgt', '--device', `127.0.0.1:${started.port}`, '--launch'], {
      cwd: dir,
    });

    const names = readZip(pushed()).map((entry) => entry.name);
    expect(names).toContain('signature1.xml');
    expect(started.opened).toContain('shell:0 execute AbCdE12345.App');
    expect(
      started.opened.some((service) => service.startsWith('shell:0 vd_appinstall AbCdE12345.App ')),
    ).toBe(true);
  });

  it('installs a package as it is signed, with --keep-signature', async () => {
    const requests: SyncRequest[] = [];
    const { tv: started, pushed } = await tv(requests);

    await installCommand.run(
      ['app.wgt', '--device', `127.0.0.1:${started.port}`, '--keep-signature'],
      { cwd: dir },
    );

    expect(readZip(pushed()).map((entry) => entry.name)).toEqual(['config.xml']);
    expect(started.opened.some((service) => service.startsWith('shell:0 execute'))).toBe(false);
  });

  it('needs exactly one package', async () => {
    await expect(installCommand.run([], { cwd: dir })).rejects.toThrow('install needs one package');
  });
});

describe('rn-tizen uninstall', () => {
  it('uninstalls a package by its id, and needs exactly one', async () => {
    const { tv: started } = await tv([]);

    await uninstallCommand.run(['AbCdE12345', '--device', `127.0.0.1:${started.port}`], {
      cwd: dir,
    });

    expect(started.opened).toContain('shell:0 appuninstall AbCdE12345');
    await expect(uninstallCommand.run([], { cwd: dir })).rejects.toThrow(
      'uninstall needs one package id',
    );
  });
});
