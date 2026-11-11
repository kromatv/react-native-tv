import { networkInterfaces } from 'node:os';
import type { DeviceCapabilities } from '../sdb/capabilities.ts';
import { SdbConnectError } from '../sdb/sdb-connection.ts';
import { openSdbDevice } from '../sdb/sdb-device.ts';
import type { FoundTv } from './scan-tvs.ts';

/** Whether sdb reaches a TV, and what it reports when it does. */
export type SdbState =
  | {
      readonly state: 'ready';
      readonly capabilities: DeviceCapabilities | null;
      readonly model: string | null;
      readonly duid: string | null;
    }
  | { readonly state: 'refused' | 'closed' | 'silent'; readonly reason: string };

/** A found TV, probed over sdb, with whether Developer mode names this computer. */
export interface TvProbe extends FoundTv {
  readonly hostPcIsThisComputer: boolean | null;
  readonly sdb: SdbState;
}

const PROBE_TIMEOUT_MS = 4000;

function localAddresses(): Set<string> {
  return new Set(
    Object.values(networkInterfaces())
      .flat()
      .flatMap((entry) => (entry?.family === 'IPv4' ? [entry.address] : [])),
  );
}

async function probeSdb(host: string, port: number): Promise<SdbState> {
  try {
    const device = await openSdbDevice({ host, port }, PROBE_TIMEOUT_MS);
    try {
      const duid = await device.duid().catch(() => null);
      return { state: 'ready', capabilities: device.capabilities, model: device.model, duid };
    } finally {
      device.close();
    }
  } catch (error) {
    const reason = error instanceof Error ? error.message : String(error);
    if (!(error instanceof SdbConnectError)) return { state: 'refused', reason };
    if (error.reason === 'unreachable') return { state: 'closed', reason };
    return { state: error.reason, reason };
  }
}

/** Connects to a TV's sdb daemon, reads its capabilities and DUID, and closes. */
export async function probeTv(tv: FoundTv): Promise<TvProbe> {
  const sdb = await probeSdb(tv.host, tv.sdbPort);
  const hostPcIsThisComputer =
    tv.developerIp === null ? null : localAddresses().has(tv.developerIp);
  const model = tv.model === '' && sdb.state === 'ready' ? (sdb.model ?? '') : tv.model;
  return { ...tv, model, hostPcIsThisComputer, sdb };
}
