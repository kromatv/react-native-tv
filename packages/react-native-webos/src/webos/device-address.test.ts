import { describe, expect, it } from 'vitest';
import { addressText, parseDeviceAddress } from './device-address.ts';

describe('parseDeviceAddress', () => {
  it('adds the Developer Mode SSH port to a bare IP address', () => {
    expect(parseDeviceAddress('192.168.1.50')).toEqual({ host: '192.168.1.50', port: 9922 });
  });

  it('keeps an explicit port', () => {
    expect(parseDeviceAddress(' 192.168.1.50:9923 ')).toEqual({
      host: '192.168.1.50',
      port: 9923,
    });
  });

  it('refuses what is not an IPv4 address', () => {
    expect(parseDeviceAddress('living-room')).toBeNull();
    expect(parseDeviceAddress('192.168.1.300')).toBeNull();
    expect(parseDeviceAddress('192.168.1.50:70000')).toBeNull();
  });
});

describe('addressText', () => {
  it('leaves out the port every TV uses', () => {
    expect(addressText({ host: '192.168.1.50', port: 9922 })).toBe('192.168.1.50');
    expect(addressText({ host: '127.0.0.1', port: 40123 })).toBe('127.0.0.1:40123');
  });
});
