import { statSync, writeFileSync } from 'node:fs';
import { describe, expect, it } from 'vitest';
import { useTemporaryState } from './connected-tv.fixtures.ts';
import { deviceKeyPath, readDeviceKey, saveDeviceKey } from './device-key.ts';

const address = { host: '192.168.1.50', port: 9922 };
const PEM = '-----BEGIN RSA PRIVATE KEY-----\nAAAA\n-----END RSA PRIVATE KEY-----\n';

useTemporaryState();

describe('device keys', () => {
  it('live in the state directory, one file per TV', () => {
    expect(deviceKeyPath(address, { XDG_STATE_HOME: '/s' })).toBe(
      '/s/react-native-webos/keys/192.168.1.50_9922.pem',
    );
  });

  it('are kept readable by this user only', () => {
    saveDeviceKey(address, PEM);

    expect(readDeviceKey(address)).toBe(PEM);
    expect(statSync(deviceKeyPath(address)).mode & 0o777).toBe(0o600);
  });

  it('read as none when the file is missing or holds no key', () => {
    expect(readDeviceKey(address)).toBeNull();

    saveDeviceKey(address, PEM);
    writeFileSync(deviceKeyPath(address), 'not a key');

    expect(readDeviceKey(address)).toBeNull();
  });
});
