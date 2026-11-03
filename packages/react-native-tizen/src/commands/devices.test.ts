import { mkdtempSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { checkDevices } from '../doctor/tizen-checks.ts';
import {
  capabilityPayload,
  fakeTv,
  TV_CAPABILITIES,
  type FakeTv,
} from '../sdb/fake-tv.fixtures.ts';
import { devicesCommand } from './devices.ts';

const tvs: FakeTv[] = [];
let state = '';
let previous: string | undefined;
let printed: string[] = [];

beforeEach(() => {
  previous = process.env.XDG_STATE_HOME;
  state = mkdtempSync(join(tmpdir(), 'rn-tizen-devices-'));
  process.env.XDG_STATE_HOME = state;
  printed = [];
  vi.spyOn(process.stdout, 'write').mockImplementation((chunk) => {
    printed.push(String(chunk));
    return true;
  });
});

afterEach(async () => {
  vi.restoreAllMocks();
  if (previous === undefined) delete process.env.XDG_STATE_HOME;
  else process.env.XDG_STATE_HOME = previous;
  rmSync(state, { recursive: true, force: true });
  await Promise.all(tvs.splice(0).map((started) => started.close()));
});

async function tv(): Promise<FakeTv> {
  const started = await fakeTv({
    services: {
      'capability:': (stream) => {
        stream.write(capabilityPayload(TV_CAPABILITIES));
        stream.close();
      },
    },
  });
  tvs.push(started);
  return started;
}

describe('rn-tizen devices', () => {
  it('says how to connect a TV when none is known', async () => {
    await devicesCommand.run([], { cwd: state });

    expect(printed.join('')).toContain('rn-tizen devices --connect <tv ip>');
  });

  it('connects a TV, remembers it and lists it with its CPU and platform', async () => {
    const started = await tv();

    await devicesCommand.run(['--connect', `127.0.0.1:${started.port}`], { cwd: state });
    printed = [];
    await devicesCommand.run([], { cwd: state });

    expect(printed.join('')).toContain('GQ75LS03DAUXZG');
    expect(printed.join('')).toContain('armv7, Tizen 9.0');
  });
});

describe('checkDevices', () => {
  it('warns when no TV is known, and names the TVs that answer', async () => {
    expect((await checkDevices()).status).toBe('warn');

    const started = await tv();
    await devicesCommand.run(['--connect', `127.0.0.1:${started.port}`], { cwd: state });

    expect(await checkDevices()).toMatchObject({
      status: 'ok',
      detail: `GQ75LS03DAUXZG at 127.0.0.1:${started.port}`,
    });
  });
});
