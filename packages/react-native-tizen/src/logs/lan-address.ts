import type { NetworkInterfaceInfo } from 'node:os';

type Interfaces = NodeJS.Dict<NetworkInterfaceInfo[]>;

function toNumber(address: string): number {
  return address.split('.').reduce((value, octet) => value * 256 + Number(octet), 0);
}

function sameSubnet(candidate: NetworkInterfaceInfo, host: string): boolean {
  const mask = toNumber(candidate.netmask);
  return (toNumber(candidate.address) & mask) >>> 0 === (toNumber(host) & mask) >>> 0;
}

function isPrivate(address: string): boolean {
  return /^(10\.|192\.168\.|172\.(1[6-9]|2\d|3[01])\.)/.test(address);
}

/**
 * This machine's IPv4 address a TV can reach: the one on the TV's subnet when
 * `deviceHost` is known, else the first private address, else any external one.
 */
export function pickLanAddress(interfaces: Interfaces, deviceHost: string | null): string | null {
  const candidates = Object.values(interfaces)
    .flatMap((entries) => entries ?? [])
    .filter((entry) => entry.family === 'IPv4' && !entry.internal)
    .filter((entry) => !entry.address.startsWith('169.254.'));
  const onDeviceSubnet =
    deviceHost === null ? undefined : candidates.find((entry) => sameSubnet(entry, deviceHost));
  const chosen =
    onDeviceSubnet ?? candidates.find((entry) => isPrivate(entry.address)) ?? candidates.at(0);
  return chosen?.address ?? null;
}
