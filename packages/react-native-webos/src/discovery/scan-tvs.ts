import { SSAP_PORTS, type SsapPorts } from '../remote/ssap-client.ts';
import { DEVELOPER_SSH_PORT } from '../webos/device-address.ts';
import { KEY_SERVER_PORT } from '../webos/key-server.ts';
import { arpHosts } from './arp.ts';
import { compareHosts, localSubnets, prefixOf, subnetHosts } from './lan.ts';
import { mapPool } from './map-pool.ts';
import { SECOND_SCREEN_SEARCH_TARGET, ssdpSearch, type SsdpReply } from './ssdp.ts';
import { tcpOpen } from './tcp-probe.ts';
import { fetchTvIdentity } from './tv-identity.ts';
import { fetchDeviceDescription, isLgTv } from './upnp.ts';

/** An LG TV found on the network, and what it said about itself. */
export interface FoundTv {
  readonly host: string;
  /** The name its owner gave it, `[LG] webOS TV <model>` by default; empty when unknown. */
  readonly name: string;
  readonly model: string;
  /** The id the TV keeps when its address changes; null when it gave none. */
  readonly id: string | null;
  /** The webOS release, such as `11.2.0`; null when its remote control service did not answer. */
  readonly webosVersion: string | null;
  /** Whether its Developer Mode SSH port is open. */
  readonly developerMode: boolean;
  /** Whether its key server answers, so `rn-webos connect` can fetch the key now. */
  readonly keyServer: boolean;
  readonly sshPort: number;
}

export interface ScanOptions {
  /** Probe these addresses only, instead of the local networks. */
  readonly hosts?: readonly string[];
  readonly onProgress?: (done: number, total: number) => void;
  /** The ports a TV answers on, LG's unless a test says otherwise. */
  readonly ports?: ScanPorts;
  readonly ssdpWindowMs?: number;
}

/** The ports that tell an LG TV and its Developer Mode: remote control, SSH and the key server. */
export interface ScanPorts {
  readonly remote: SsapPorts;
  readonly ssh: number;
  readonly keyServer: number;
}

const DEFAULT_PORTS: ScanPorts = {
  remote: SSAP_PORTS,
  ssh: DEVELOPER_SSH_PORT,
  keyServer: KEY_SERVER_PORT,
};

const CONNECT_TIMEOUT_MS = 500;
const CONCURRENCY = 64;
const SSDP_WINDOW_MS = 2500;

/** The hosts of this machine's own `/24`s, the ones it talked to lately first. */
export async function sweepTargets(): Promise<string[]> {
  const subnets = localSubnets();
  const prefixes = new Set(subnets.map((subnet) => subnet.prefix));
  const recent = (await arpHosts()).filter((host) => prefixes.has(prefixOf(host) ?? ''));
  const rest = subnets.flatMap((subnet) => subnetHosts(subnet.prefix, subnet.address));
  return [...new Set([...recent, ...rest])];
}

async function anyOpen(host: string, ports: readonly number[]): Promise<boolean> {
  const [port, ...rest] = ports;
  if (port === undefined) return false;
  return (await tcpOpen(host, port, CONNECT_TIMEOUT_MS)) || anyOpen(host, rest);
}

async function identify(
  host: string,
  reply: SsdpReply | undefined,
  ports: ScanPorts,
): Promise<FoundTv | null> {
  const [described, identity, developerMode, keyServer] = await Promise.all([
    reply === undefined ? null : fetchDeviceDescription(reply.location),
    fetchTvIdentity(host, ports.remote),
    tcpOpen(host, ports.ssh, CONNECT_TIMEOUT_MS),
    tcpOpen(host, ports.keyServer, CONNECT_TIMEOUT_MS),
  ]);
  const description = described !== null && isLgTv(described) ? described : null;
  if (description === null && identity === null && !developerMode) return null;
  return {
    host,
    name: description?.friendlyName ?? '',
    model: description?.modelNumber || identity?.model || '',
    id: identity?.uuid ?? (description?.udn || reply?.uuid) ?? null,
    webosVersion: identity?.releaseVersion ?? null,
    developerMode,
    keyServer,
    sshPort: ports.ssh,
  };
}

/**
 * Finds the LG TVs on the local networks: an SSDP search for their remote
 * control service, then every host of each `/24`, tried on the remote control
 * ports and the Developer Mode SSH port, for networks that drop multicast.
 */
export async function scanForTvs(options: ScanOptions = {}): Promise<FoundTv[]> {
  const ports = options.ports ?? DEFAULT_PORTS;
  const searching = ssdpSearch(options.ssdpWindowMs ?? SSDP_WINDOW_MS);
  const targets = options.hosts === undefined ? await sweepTargets() : [...options.hosts];
  const sweptPorts = [ports.remote.secure, ports.remote.plain, ports.ssh];
  let done = 0;
  const swept = await mapPool(targets, CONCURRENCY, async (host) => {
    const open = await anyOpen(host, sweptPorts);
    options.onProgress?.(++done, targets.length);
    return open ? host : null;
  });
  const replies = new Map(
    (await searching)
      .filter((reply) => reply.target === SECOND_SCREEN_SEARCH_TARGET)
      .map((reply) => [reply.host, reply]),
  );
  const candidates = new Set([...swept.filter((host) => host !== null), ...replies.keys()]);
  if (options.hosts !== undefined)
    for (const host of candidates) if (!options.hosts.includes(host)) candidates.delete(host);
  const found = await mapPool([...candidates], 8, (host) =>
    identify(host, replies.get(host), ports),
  );
  return found
    .filter((tv) => tv !== null)
    .sort((left, right) => compareHosts(left.host, right.host));
}
