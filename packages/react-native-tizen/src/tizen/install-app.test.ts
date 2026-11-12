import { mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, describe, expect, it } from 'vitest';
import {
  acceptPush,
  capabilityPayload,
  fakeTv,
  TV_CAPABILITIES,
  type FakeTv,
  type ServiceHandler,
} from '../sdb/fake-tv.fixtures.ts';
import { SDB_COMMAND } from '../sdb/packet.ts';
import { probeDevice } from './devices.ts';
import {
  installApp,
  installFailureReason,
  installOn,
  launchApp,
  launchOn,
  onDevice,
  uninstallApp,
} from './install-app.ts';

const tvs: FakeTv[] = [];
const dirs: string[] = [];
const task = { run: () => Promise.resolve(''), progress: () => undefined };
const CERTIFICATE_FAILURE =
  'app_id[org.example] install failed[118, -12], reason: Check certificate error : :Invalid signature.:<-4>';

function answer(text: string): ServiceHandler {
  return (stream) => {
    stream.write(text);
    stream.close();
  };
}

async function device(services: Record<string, ServiceHandler>, holdMs?: number) {
  const started = await fakeTv({
    holdMs,
    services: {
      'capability:': (stream) => {
        stream.write(capabilityPayload(TV_CAPABILITIES));
        stream.close();
      },
      'sync:': acceptPush([]),
      'shell:0 getduid': answer('ABCDEFGH12345\r\n'),
      ...services,
    },
  });
  tvs.push(started);
  return probeDevice({ host: '127.0.0.1', port: started.port });
}

function tpk(): string {
  const dir = mkdtempSync(join(tmpdir(), 'rn-tizen-install-'));
  dirs.push(dir);
  const path = join(dir, 'org.example-1.0.0.tpk');
  writeFileSync(path, 'package');
  return path;
}

afterEach(async () => {
  await Promise.all(tvs.splice(0).map((started) => started.close()));
  for (const dir of dirs.splice(0)) rmSync(dir, { recursive: true, force: true });
});

describe('installFailureReason', () => {
  it('keeps the reason a TV gives for refusing a package', () => {
    expect(installFailureReason(`installing[18]\n${CERTIFICATE_FAILURE}\nspend time`)).toBe(
      'Check certificate error : :Invalid signature.:<-4>',
    );
  });

  it('falls back to the last line printed', () => {
    expect(installFailureReason('first\nsomething broke\n\n')).toBe('something broke');
    expect(installFailureReason('')).toBe('no output');
  });
});

describe('installApp', () => {
  it('installs what the TV accepts', async () => {
    const tv = await device({
      'shell:0 vd_appinstall': answer('app_id[org.example] install completed\n'),
    });

    await expect(
      installApp(task, tv, { file: tpk(), installId: 'org.example' }),
    ).resolves.toBeUndefined();
  });

  it('names the DUID when the TV refuses the signature', async () => {
    const tv = await device({ 'shell:0 vd_appinstall': answer(CERTIFICATE_FAILURE) });

    await expect(
      installApp(task, tv, { file: tpk(), installId: 'org.example' }),
    ).rejects.toMatchObject({
      message: expect.stringContaining('Invalid signature'),
      hint: expect.stringContaining('ABCDEFGH12345'),
    });
  });

  it('points at Developer mode for any other failure', async () => {
    const tv = await device({
      'shell:0 vd_appinstall': answer('install failed[1, -1], reason: No space'),
    });

    await expect(
      installApp(task, tv, { file: tpk(), installId: 'org.example' }),
    ).rejects.toMatchObject({
      hint: expect.stringContaining('Developer mode'),
    });
  });
});

describe('installApp and the DUID', () => {
  it('stops before sending a package whose certificate does not list the TV', async () => {
    const tv = await device({ 'shell:0 vd_appinstall': answer('install completed') });

    const installing = installApp(task, tv, {
      file: tpk(),
      installId: 'org.example',
      duids: ['OTHER0000000A'],
    });

    await expect(installing).rejects.toThrow('does not list');
    await expect(installing).rejects.toMatchObject({
      hint: expect.stringContaining('OTHER0000000A'),
    });
  });

  it('installs when the certificate lists the TV', async () => {
    const tv = await device({ 'shell:0 vd_appinstall': answer('install completed') });

    await expect(
      installApp(task, tv, { file: tpk(), installId: 'org.example', duids: ['ABCDEFGH12345'] }),
    ).resolves.toBeUndefined();
  });
});

describe('uninstallApp', () => {
  it('uninstalls with appuninstall, and reports what the TV said when it does not', async () => {
    const removed = await device({
      'shell:0 appuninstall': answer('app_id[org.example] uninstall completed\n'),
    });
    const refused = await device({ 'shell:0 appuninstall': answer('uninstall failed') });

    await expect(uninstallApp(removed, 'org.example')).resolves.toBeUndefined();
    await expect(uninstallApp(refused, 'org.example')).rejects.toThrow(
      'did not uninstall org.example',
    );
  });
});

describe('launchApp', () => {
  it('launches, and reports what the TV said when it does not', async () => {
    const launched = await device({
      'shell:0 execute': answer('... successfully launched pid = 7 with debug 0\r\n'),
    });
    const missing = await device({
      'shell:0 execute': answer('The app with ID: org.example is not available'),
    });

    await expect(launchApp(launched, 'org.example')).resolves.toBeUndefined();
    await expect(launchApp(missing, 'org.example')).rejects.toThrow('did not launch org.example');
  });
});

describe('install, then launch', () => {
  it('chains both on one connection, right after the check, on a TV that holds its last one', async () => {
    const tv = await device(
      {
        'shell:0 vd_appinstall': answer('install completed\n'),
        'shell:0 execute': answer('successfully launched\n'),
      },
      200,
    );
    const started = tvs.at(-1);

    await onDevice(tv, async (open) => {
      await installOn(open, task, tv, { file: tpk(), installId: 'org.example' });
      await launchOn(open, tv, 'org.example.app');
    });
    await launchApp(tv, 'org.example.app');

    const connections = started?.packets.filter((packet) => packet.command === SDB_COMMAND.CNXN);
    expect(connections).toHaveLength(3);
    expect(started?.opened.filter((service) => service.startsWith('shell:0 execute'))).toHaveLength(
      2,
    );
  });
});
