import type { NetworkInterfaceInfo } from 'node:os';
import { describe, expect, it } from 'vitest';
import { pickLanAddress } from './lan-address.ts';

const ipv4 = (
  address: string,
  netmask = '255.255.255.0',
  internal = false,
): NetworkInterfaceInfo => ({
  address,
  netmask,
  family: 'IPv4',
  mac: '00:00:00:00:00:00',
  internal,
  cidr: null,
});

const INTERFACES = {
  lo0: [ipv4('127.0.0.1', '255.0.0.0', true)],
  utun4: [ipv4('100.64.0.2', '255.192.0.0')],
  en7: [ipv4('10.0.0.5', '255.255.255.0')],
  en0: [ipv4('192.168.1.20')],
};

describe('pickLanAddress', () => {
  it('picks the address on the TV subnet', () => {
    expect(pickLanAddress(INTERFACES, '192.168.1.50')).toBe('192.168.1.20');
  });

  it('falls back to a private address when no interface shares the TV subnet', () => {
    expect(pickLanAddress(INTERFACES, '172.20.0.9')).toBe('10.0.0.5');
  });

  it('picks a private address when the TV is unknown', () => {
    expect(pickLanAddress({ utun4: INTERFACES.utun4, en0: INTERFACES.en0 }, null)).toBe(
      '192.168.1.20',
    );
  });

  it('never picks a loopback or link-local address', () => {
    expect(pickLanAddress({ lo0: INTERFACES.lo0, en5: [ipv4('169.254.3.4')] }, null)).toBeNull();
  });
});
