import { mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, describe, expect, it } from 'vitest';
import {
  isDeviceName,
  knownDevicesPath,
  readKnownDevices,
  rememberDevice,
} from './known-devices.ts';

const dirs: string[] = [];
const livingRoom = { name: 'living-room', host: '192.168.1.50', port: 9922 };
const office = { name: 'OLED65C25LB', host: '192.168.1.60', port: 9922 };

function file(): string {
  const dir = mkdtempSync(join(tmpdir(), 'rn-webos-devices-'));
  dirs.push(dir);
  return join(dir, 'state', 'devices.json');
}

afterEach(() => {
  for (const dir of dirs.splice(0)) rmSync(dir, { recursive: true, force: true });
});

describe('known devices', () => {
  it('lives in the state directory', () => {
    expect(knownDevicesPath({ XDG_STATE_HOME: '/s' })).toBe('/s/react-native-webos/devices.json');
  });

  it('remembers the last TV connected first, once, under its latest name', async () => {
    const path = file();

    await rememberDevice(livingRoom, path);
    await rememberDevice(office, path);
    await rememberDevice({ ...livingRoom, name: 'lounge' }, path);

    expect(await readKnownDevices(path)).toEqual([{ ...livingRoom, name: 'lounge' }, office]);
  });

  it('reads a missing or malformed file as no TV', async () => {
    const path = file();
    expect(await readKnownDevices(path)).toEqual([]);

    await rememberDevice(livingRoom, path);
    writeFileSync(path, '{"devices":[{"name":"tv","host":"not an ip","port":1}]}');

    expect(await readKnownDevices(path)).toEqual([]);
  });
});

describe('isDeviceName', () => {
  it('takes a model or a word, and nothing --device would read as an address', () => {
    expect(isDeviceName('OLED65G56LS.DEUQLJP')).toBe(true);
    expect(isDeviceName('living-room')).toBe(true);
    expect(isDeviceName('192.168.1.50')).toBe(false);
    expect(isDeviceName('[LG] webOS TV')).toBe(false);
    expect(isDeviceName('')).toBe(false);
  });
});
