import { mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, describe, expect, it } from 'vitest';
import { knownDevicesPath, readKnownDevices, rememberDevice } from './known-devices.ts';

const dirs: string[] = [];

function file(): string {
  const dir = mkdtempSync(join(tmpdir(), 'rn-tizen-devices-'));
  dirs.push(dir);
  return join(dir, 'state', 'devices.json');
}

afterEach(() => {
  for (const dir of dirs.splice(0)) rmSync(dir, { recursive: true, force: true });
});

describe('known devices', () => {
  it('lives in the state directory', () => {
    expect(knownDevicesPath({ XDG_STATE_HOME: '/s' })).toBe('/s/react-native-tizen/devices.json');
  });

  it('remembers the last TV reached first, once', async () => {
    const path = file();

    await rememberDevice({ host: '192.168.1.50', port: 26101 }, path);
    await rememberDevice({ host: '192.168.1.60', port: 26101 }, path);
    await rememberDevice({ host: '192.168.1.50', port: 26101 }, path);

    expect(await readKnownDevices(path)).toEqual([
      { host: '192.168.1.50', port: 26101 },
      { host: '192.168.1.60', port: 26101 },
    ]);
  });

  it('reads a missing or malformed file as no TV', async () => {
    const path = file();
    expect(await readKnownDevices(path)).toEqual([]);

    await rememberDevice({ host: '192.168.1.50', port: 26101 }, path);
    writeFileSync(path, '{"devices":[{"host":"not an ip","port":1}]}');

    expect(await readKnownDevices(path)).toEqual([]);
  });
});
