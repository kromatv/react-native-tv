import type { DeviceInfo } from '../webos/device-info.ts';
import { probeDevice } from '../webos/devices.ts';
import { readKnownDevices } from '../webos/known-devices.ts';
import type { FoundTv } from './scan-tvs.ts';

/** Whether `rn-webos` can log in to a TV: it can, it was never connected, or why it cannot. */
export type ConnectionState =
  | { readonly state: 'ready'; readonly name: string; readonly info: DeviceInfo }
  | { readonly state: 'unknown' }
  | { readonly state: 'failed'; readonly name: string; readonly reason: string };

/** A found TV, with whether `rn-webos` gets in over SSH. */
export interface TvProbe extends FoundTv {
  readonly connection: ConnectionState;
}

/** Logs in to a TV `rn-webos` was connected to before, reads what it reports, and closes. */
export async function probeTv(tv: FoundTv): Promise<TvProbe> {
  const known = (await readKnownDevices()).find(
    (each) => each.host === tv.host && each.port === tv.sshPort,
  );
  if (known === undefined) return { ...tv, connection: { state: 'unknown' } };
  try {
    const { name, info } = await probeDevice(known);
    const model = tv.model === '' ? info.modelName : tv.model;
    const webosVersion = tv.webosVersion ?? (info.sdkVersion === '' ? null : info.sdkVersion);
    return { ...tv, model, webosVersion, connection: { state: 'ready', name, info } };
  } catch (error) {
    const reason = error instanceof Error ? error.message : String(error);
    return { ...tv, connection: { state: 'failed', name: known.name, reason } };
  }
}
