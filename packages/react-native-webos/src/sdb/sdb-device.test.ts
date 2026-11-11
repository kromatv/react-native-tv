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
  type SyncRequest,
} from './fake-tv.fixtures.ts';
import { SDB_COMMAND } from './packet.ts';
import { SdbConnectError } from './sdb-connection.ts';
import { openSdbDevice, SdbCommandError, withSdbDevice } from './sdb-device.ts';
import { PUSH_MODE } from './sync-push.ts';

const tvs: FakeTv[] = [];
const dirs: string[] = [];

const capability: ServiceHandler = (stream) => {
  stream.write(capabilityPayload(TV_CAPABILITIES));
  stream.close();
};

function answer(text: string): ServiceHandler {
  return (stream) => {
    stream.write(text);
    stream.close();
  };
}

async function tv(services: Record<string, ServiceHandler>, handshake?: 'reset' | 'silent') {
  const started = await fakeTv({ services: { 'capability:': capability, ...services }, handshake });
  tvs.push(started);
  return started;
}

function packageFile(bytes: Buffer): string {
  const dir = mkdtempSync(join(tmpdir(), 'rn-tizen-sdb-'));
  dirs.push(dir);
  const path = join(dir, 'org.example-1.0.0.tpk');
  writeFileSync(path, bytes);
  return path;
}

afterEach(async () => {
  await Promise.all(tvs.splice(0).map((started) => started.close()));
  for (const dir of dirs.splice(0)) rmSync(dir, { recursive: true, force: true });
});

describe('openSdbDevice', () => {
  it('reads the model from the banner and the capabilities from their service', async () => {
    const started = await tv({});

    const device = await openSdbDevice({ host: '127.0.0.1', port: started.port });
    device.close();

    expect(device.model).toBe('GQ75LS03DAUXZG');
    expect(device.capabilities?.cpuArch).toBe('armv7');
    expect(started.packets[0]?.arg0).toBe(0x00100000);
  });

  it('calls a connection the TV keeps resetting refused, after about two seconds of tries', async () => {
    const started = await tv({}, 'reset');
    const startedAt = Date.now();

    const opening = openSdbDevice({ host: '127.0.0.1', port: started.port }, 2000);

    await expect(opening).rejects.toBeInstanceOf(SdbConnectError);
    await expect(opening).rejects.toMatchObject({ reason: 'refused' });
    expect(Date.now() - startedAt).toBeGreaterThanOrEqual(1800);
    expect(Date.now() - startedAt).toBeLessThan(5000);
  });

  it('connects right behind its own last connection, which the TV still holds', async () => {
    const started = await fakeTv({ services: { 'capability:': capability }, holdMs: 200 });
    tvs.push(started);
    const address = { host: '127.0.0.1', port: started.port };
    const probe = await openSdbDevice(address, 2000);
    probe.close();

    const install = await openSdbDevice(address, 2000);
    install.close();

    expect([probe.model, install.model]).toEqual(['GQ75LS03DAUXZG', 'GQ75LS03DAUXZG']);
  });

  it('calls a TV that never sends its banner silent', async () => {
    const started = await tv({}, 'silent');

    await expect(
      openSdbDevice({ host: '127.0.0.1', port: started.port }, 300),
    ).rejects.toMatchObject({
      reason: 'silent',
    });
  });

  it('calls a port nobody listens on unreachable', async () => {
    const started = await tv({});
    await started.close();

    await expect(
      openSdbDevice({ host: '127.0.0.1', port: started.port }, 2000),
    ).rejects.toMatchObject({
      reason: 'unreachable',
    });
  });
});

describe('install', () => {
  it('pushes the package under tmp/ of the tool path, then installs it with vd_appinstall', async () => {
    const requests: SyncRequest[] = [];
    const bytes = Buffer.alloc(300_000, 7);
    const started = await tv({
      'sync:': acceptPush(requests),
      'shell:0 vd_appinstall': answer(
        'app_id[org.example] installing[50]\napp_id[org.example] install completed\n',
      ),
    });
    const progress: string[] = [];

    await withSdbDevice({ host: '127.0.0.1', port: started.port }, (device) =>
      device.install(packageFile(bytes), 'org.example', (step) =>
        progress.push(`${step.phase} ${step.percent}`),
      ),
    );

    const remote = '/home/owner/share/tmp/sdk_tools/tmp/org.example-1.0.0.tpk';
    expect(requests[0]?.payload.toString()).toBe(`${remote},${PUSH_MODE}`);
    const data = requests.filter((request) => request.id === 'DATA');
    expect(data.every((request) => request.value <= 64 * 1024)).toBe(true);
    expect(Buffer.concat(data.map((request) => request.payload)).equals(bytes)).toBe(true);
    expect(started.opened.at(-1)).toBe(`shell:0 vd_appinstall org.example ${remote}`);
    expect(progress).toContain('push 100');
    expect(progress).toContain('install 50');
  });

  it('packs the push into writes no larger than the TV takes', async () => {
    const started = await tv({
      'sync:': acceptPush([]),
      'shell:0 vd_appinstall': answer('install completed'),
    });

    await withSdbDevice({ host: '127.0.0.1', port: started.port }, (device) =>
      device.install(packageFile(Buffer.alloc(600_000, 1)), 'org.example'),
    );

    const writes = started.packets.filter((packet) => packet.command === SDB_COMMAND.WRTE);
    expect(Math.max(...writes.map((packet) => packet.data.length))).toBe(0x3ff9c);
  });

  it('fails with what the TV printed when the install fails', async () => {
    const failed = 'app_id[org.example] install failed[118, -12], reason: Check certificate error';
    const started = await tv({ 'sync:': acceptPush([]), 'shell:0 vd_appinstall': answer(failed) });

    const installing = withSdbDevice({ host: '127.0.0.1', port: started.port }, (device) =>
      device.install(packageFile(Buffer.from('tpk')), 'org.example'),
    );

    await expect(installing).rejects.toBeInstanceOf(SdbCommandError);
    await expect(installing).rejects.toMatchObject({ output: failed });
  });

  it('reports the message of a push the TV refuses', async () => {
    const message = Buffer.from('No space left on device');
    const reply = Buffer.concat([
      Buffer.from('FAIL'),
      Buffer.from([message.length, 0, 0, 0]),
      message,
    ]);
    const started = await tv({ 'sync:': acceptPush([], reply) });

    const installing = withSdbDevice({ host: '127.0.0.1', port: started.port }, (device) =>
      device.install(packageFile(Buffer.from('tpk')), 'org.example'),
    );

    await expect(installing).rejects.toThrow('No space left on device');
  });
});

describe('launch and duid', () => {
  it('launches with execute and reads the DUID with getduid', async () => {
    const started = await tv({
      'shell:0 execute': answer('... successfully launched pid = 30987 with debug 0\r\n'),
      'shell:0 getduid': answer('ABCDEFGH12345\r\n'),
    });

    const duid = await withSdbDevice({ host: '127.0.0.1', port: started.port }, async (device) => {
      await device.launch('org.example');
      return device.duid();
    });

    expect(started.opened).toContain('shell:0 execute org.example');
    expect(duid).toBe('ABCDEFGH12345');
  });

  it('fails a launch the TV answers without a pid', async () => {
    const started = await tv({
      'shell:0 execute': answer('The app with ID: org.example is not available'),
    });

    await expect(
      withSdbDevice({ host: '127.0.0.1', port: started.port }, (device) =>
        device.launch('org.example'),
      ),
    ).rejects.toThrow('did not launch');
  });

  it('fails a service the TV refuses', async () => {
    const started = await tv({});

    await expect(
      withSdbDevice({ host: '127.0.0.1', port: started.port }, (device) =>
        device.launch('org.example'),
      ),
    ).rejects.toThrow('refused shell:0 execute');
  });
});
