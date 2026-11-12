import { SDB_PORT } from '../sdb/sdb-connection.ts';
import { arpHosts } from './arp.ts';
import { compareHosts, localSubnets, prefixOf, subnetHosts } from './lan.ts';
import { mapPool } from './map-pool.ts';
import { fetchApiDevice, SAMSUNG_API_PORT } from './samsung-api.ts';
import { ssdpSearch, type SsdpReply } from './ssdp.ts';
import { tcpOpen } from './tcp-probe.ts';
import { fetchDeviceDescription } from './upnp.ts';

/** A Samsung TV found on the network, and what it said about itself. */
export interface FoundTv {
  readonly host: string;
  readonly name: string;
  readonly model: string;
  /** Null when the TV's REST API did not answer, so Developer mode is unknown. */
  readonly developerMode: boolean | null;
  readonly developerIp: string | null;
  readonly sdbPort: number;
}

export interface ScanOptions {
  /** Probe these addresses only, instead of the local networks. */
  readonly hosts?: readonly string[];
  readonly onProgress?: (done: number, total: number) => void;
  /** The ports a TV answers on, Samsung's REST API and sdb unless a test says otherwise. */
  readonly ports?: ScanPorts;
  readonly ssdpWindowMs?: number;
}

interface ScanPorts {
  readonly api: number;
  readonly sdb: number;
}

const DEFAULT_PORTS: ScanPorts = { api: SAMSUNG_API_PORT, sdb: SDB_PORT };

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

async function identify(
  host: string,
  reply: SsdpReply | undefined,
  ports: ScanPorts,
): Promise<FoundTv | null> {
  if (await tcpOpen(host, ports.api, CONNECT_TIMEOUT_MS)) {
    const device = await fetchApiDevice(host, ports.api);
    if (device !== null) {
      return {
        host,
        name: device.name,
        model: device.modelName,
        developerMode: device.developerMode,
        developerIp: device.developerIp,
        sdbPort: ports.sdb,
      };
    }
  }
  const description = reply === undefined ? null : await fetchDeviceDescription(reply.location);
  if (description?.manufacturer.startsWith('Samsung') === true) {
    return {
      host,
      name: description.friendlyName,
      model: description.modelName,
      developerMode: null,
      developerIp: null,
      sdbPort: ports.sdb,
    };
  }
  if (await tcpOpen(host, ports.sdb, CONNECT_TIMEOUT_MS)) {
    return {
      host,
      name: '',
      model: '',
      developerMode: true,
      developerIp: null,
      sdbPort: ports.sdb,
    };
  }
  return null;
}

/**
 * Finds the Samsung TVs on the local networks: an SSDP search for DIAL, then
 * every host of each `/24`, asked on Samsung's REST API port, its UPnP
 * description, or the sdb port.
 */
export async function scanForTvs(options: ScanOptions = {}): Promise<FoundTv[]> {
  const ports = options.ports ?? DEFAULT_PORTS;
  const searching = ssdpSearch(options.ssdpWindowMs ?? SSDP_WINDOW_MS);
  const targets = options.hosts === undefined ? await sweepTargets() : [...options.hosts];
  let done = 0;
  const swept = await mapPool(targets, CONCURRENCY, async (host) => {
    const open = await tcpOpen(host, ports.api, CONNECT_TIMEOUT_MS);
    const sdb = open ? false : await tcpOpen(host, ports.sdb, CONNECT_TIMEOUT_MS);
    options.onProgress?.(++done, targets.length);
    return open || sdb ? host : null;
  });
  const replies = new Map(
    (await searching)
      .filter((reply) => reply.server.includes('Samsung'))
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
