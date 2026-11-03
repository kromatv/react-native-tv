import { networkInterfaces, type NetworkInterfaceInfo } from 'node:os';

/** An IPv4 network this machine sits on: its interface, its own address and the `/24` prefix. */
export interface Subnet {
  readonly iface: string;
  readonly address: string;
  readonly prefix: string;
}

const VIRTUAL_INTERFACE = /^(bridge|utun|awdl|llw|anpi|ap\d|docker|virbr|br-|tun|tap|veth|zt|wg)/;
const OCTET = /^\d{1,3}$/;

/** `192.168.1.42` gives `192.168.1`; anything not dotted-quad IPv4 gives null. */
export function prefixOf(address: string): string | null {
  const octets = address.split('.');
  if (octets.length !== 4 || !octets.every((octet) => OCTET.test(octet) && Number(octet) <= 255))
    return null;
  return octets.slice(0, 3).join('.');
}

type InterfaceAddress = Pick<NetworkInterfaceInfo, 'address' | 'family' | 'internal'>;

/** Every IPv4 network of a physical interface, one per `/24`. */
export function localSubnets(
  interfaces: Readonly<
    Record<string, readonly InterfaceAddress[] | undefined>
  > = networkInterfaces(),
): Subnet[] {
  const subnets: Subnet[] = [];
  for (const [iface, entries] of Object.entries(interfaces)) {
    if (VIRTUAL_INTERFACE.test(iface)) continue;
    for (const entry of entries ?? []) {
      const prefix = entry.family === 'IPv4' && !entry.internal ? prefixOf(entry.address) : null;
      if (prefix !== null && !subnets.some((subnet) => subnet.prefix === prefix)) {
        subnets.push({ iface, address: entry.address, prefix });
      }
    }
  }
  return subnets;
}

/** The hosts of a `/24`, but `self`. A home network is one; a wider sweep takes longer than anyone waits. */
export function subnetHosts(prefix: string, self?: string): string[] {
  const hosts: string[] = [];
  for (let last = 1; last <= 254; last++) {
    const host = `${prefix}.${last}`;
    if (host !== self) hosts.push(host);
  }
  return hosts;
}

function hostKey(host: string): number {
  return host.split('.').reduce((key, octet) => key * 256 + Number(octet), 0);
}

/** IPv4 addresses in numeric order. */
export function compareHosts(left: string, right: string): number {
  return hostKey(left) - hostKey(right);
}
