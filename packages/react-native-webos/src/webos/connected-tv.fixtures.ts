import { mkdtempSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach } from 'vitest';
import { saveDeviceKey } from './device-key.ts';
import type { Device } from './devices.ts';
import { fakeTv, type FakeTv, type FakeTvOptions } from './fake-tv.fixtures.ts';
import { rememberDevice } from './known-devices.ts';

/** Points `rn-webos` at an empty state directory for each test, and returns where it is. */
export function useTemporaryState(): () => string {
  let state = '';
  let previous: string | undefined;
  beforeEach(() => {
    previous = process.env.XDG_STATE_HOME;
    state = mkdtempSync(join(tmpdir(), 'rn-webos-state-'));
    process.env.XDG_STATE_HOME = state;
  });
  afterEach(() => {
    if (previous === undefined) delete process.env.XDG_STATE_HOME;
    else process.env.XDG_STATE_HOME = previous;
    rmSync(state, { recursive: true, force: true });
  });
  return () => state;
}

/** Starts fake TVs that `rn-webos` is already connected to, and closes them after each test. */
export function useFakeTvs(): (
  options?: FakeTvOptions & { name?: string },
) => Promise<ConnectedTv> {
  const started: FakeTv[] = [];
  afterEach(async () => {
    await Promise.all(started.splice(0).map((tv) => tv.close()));
  });
  return async (options = {}) => {
    const tv = await fakeTv(options);
    started.push(tv);
    const address = { host: '127.0.0.1', port: tv.port };
    const name = options.name ?? `tv${started.length}`;
    saveDeviceKey(address, tv.privateKey);
    await rememberDevice({ name, ...address });
    const info = { modelName: 'OLED65C25LB', sdkVersion: '11.2.0', firmwareVersion: '43.21.74' };
    return { tv, device: { address, name, info } };
  };
}

export interface ConnectedTv {
  readonly tv: FakeTv;
  readonly device: Device;
}
