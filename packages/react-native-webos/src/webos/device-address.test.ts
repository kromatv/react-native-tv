import { describe, expect, it } from 'vitest';
import { deviceSerial, parseDeviceAddress } from './device-address.ts';

describe('parseDeviceAddress', () => {
  it('adds the sdb port to a bare IP address', () => {
    expect(parseDeviceAddress('192.168.1.50')).toEqual({ host: '192.168.1.50', port: 26101 });
  });

  it('keeps an explicit port', () => {
    expect(parseDeviceAddress(' 192.168.1.50:26102 ')).toEqual({
      host: '192.168.1.50',
      port: 26102,
    });
  });

  it('refuses what is not an IPv4 address', () => {
    expect(parseDeviceAddress('emulator-26101')).toBeNull();
    expect(parseDeviceAddress('192.168.1.300')).toBeNull();
    expect(parseDeviceAddress('192.168.1.50:70000')).toBeNull();
  });
});

describe('deviceSerial', () => {
  it('names a TV host:port, as sdb does', () => {
    expect(deviceSerial({ host: '192.168.1.50', port: 26101 })).toBe('192.168.1.50:26101');
  });
});
