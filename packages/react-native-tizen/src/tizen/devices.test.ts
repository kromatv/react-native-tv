import { mkdtempSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { FixableError } from '../errors.ts';
import {
  capabilityPayload,
  fakeTv,
  TV_CAPABILITIES,
  type FakeTv,
  type ServiceHandler,
} from '../sdb/fake-tv.fixtures.ts';
import { SdbConnectError } from '../sdb/sdb-connection.ts';
import { connectFailure, listDevices, probeDevice, resolveDevice } from './devices.ts';
import { readKnownDevices, rememberDevice } from './known-devices.ts';

const tvs: FakeTv[] = [];
let state = '';
let previous: string | undefined;

const capability: ServiceHandler = (stream) => {
  stream.write(capabilityPayload(TV_CAPABILITIES));
  stream.close();
};

async function tv(handshake?: 'reset'): Promise<FakeTv> {
  const started = await fakeTv({ services: { 'capability:': capability }, handshake });
  tvs.push(started);
  return started;
}

beforeEach(() => {
  previous = process.env.XDG_STATE_HOME;
  state = mkdtempSync(join(tmpdir(), 'rn-tizen-state-'));
  process.env.XDG_STATE_HOME = state;
});

afterEach(async () => {
  if (previous === undefined) delete process.env.XDG_STATE_HOME;
  else process.env.XDG_STATE_HOME = previous;
  rmSync(state, { recursive: true, force: true });
  await Promise.all(tvs.splice(0).map((started) => started.close()));
});

describe('probeDevice', () => {
  it('names the TV by the model in its banner and reads its CPU and platform', async () => {
    const started = await tv();

    const device = await probeDevice({ host: '127.0.0.1', port: started.port });

    expect(device).toMatchObject({
      serial: `127.0.0.1:${started.port}`,
      name: 'GQ75LS03DAUXZG',
      capabilities: { cpuArch: 'armv7', platformVersion: '9.0' },
    });
  });

  it('explains a TV that resets the connection as one held by another host', async () => {
    const started = await tv('reset');

    const probing = probeDevice({ host: '127.0.0.1', port: started.port }, 2000);

    await expect(probing).rejects.toBeInstanceOf(FixableError);
    await expect(probing).rejects.toThrow('refused the connection');
  });
});

describe('connectFailure', () => {
  it('keeps an error that is not a connection failure', () => {
    const error = new Error('disk full');

    expect(connectFailure('x', error)).toBe(error);
    expect(connectFailure('x', 'text').message).toBe('text');
    expect(connectFailure('x', new SdbConnectError('silent', 'quiet')).message).toContain(
      'never answered',
    );
  });
});

describe('resolveDevice', () => {
  it('remembers a TV reached by address, and finds it again without one', async () => {
    const started = await tv();

    await resolveDevice(`127.0.0.1:${started.port}`);
    const again = await resolveDevice(undefined);

    expect(await readKnownDevices()).toEqual([{ host: '127.0.0.1', port: started.port }]);
    expect(again.serial).toBe(`127.0.0.1:${started.port}`);
  });

  it('refuses what is not an address, and asks for one when no TV answers', async () => {
    await expect(resolveDevice('living-room')).rejects.toThrow('is not a TV address');
    await expect(resolveDevice(undefined)).rejects.toThrow('No TV is connected');
  });

  it('asks which TV when several answer', async () => {
    const first = await tv();
    const second = await tv();
    await rememberDevice({ host: '127.0.0.1', port: first.port });
    await rememberDevice({ host: '127.0.0.1', port: second.port });

    await expect(resolveDevice(undefined)).rejects.toThrow('2 TVs are connected');
  });
});

describe('listDevices', () => {
  it('reports a remembered TV that does not answer as offline', async () => {
    const started = await tv();
    await started.close();
    await rememberDevice({ host: '127.0.0.1', port: started.port });

    const [status] = await listDevices();

    expect(status?.state).toBe('offline');
  });
});
