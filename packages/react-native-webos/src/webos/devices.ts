import { FixableError } from '../errors.ts';
import { addressText, parseDeviceAddress, type TvAddress } from './device-address.ts';
import { NO_DEVICE_INFO, readDeviceInfo, type DeviceInfo } from './device-info.ts';
import { readKnownDevices, rememberDevice, type KnownDevice } from './known-devices.ts';
import { DEVELOPER_MODE_HINT, openTvSession } from './tv-session.ts';

/** A TV that answered over SSH, with what it reported about itself. */
export interface Device {
  readonly address: TvAddress;
  /** The name it was connected under. */
  readonly name: string;
  readonly info: DeviceInfo;
}

/** A remembered TV, probed: online with its details, or why it did not answer. */
export type DeviceStatus =
  | { readonly known: KnownDevice; readonly state: 'online'; readonly device: Device }
  | {
      readonly known: KnownDevice;
      readonly state: 'offline';
      readonly reason: string;
      /** What to do about it, when `rn-webos` knows. */
      readonly hint: string | null;
    };

const PROBE_TIMEOUT_MS = 8000;
/** What to do on a TV before `rn-webos` can reach it. */
export const CONNECT_HINT = `${DEVELOPER_MODE_HINT} Then turn Key Server on there and run: rn-webos connect <tv ip>`;

/** Logs in to a remembered TV and reads what it reports. */
export async function probeDevice(
  known: KnownDevice,
  timeoutMs = PROBE_TIMEOUT_MS,
): Promise<Device> {
  const address = { host: known.host, port: known.port };
  const session = await openTvSession(address, timeoutMs);
  try {
    const info = await readDeviceInfo(session).catch(() => NO_DEVICE_INFO);
    return { address, name: known.name, info };
  } finally {
    session.close();
  }
}

/** Every remembered TV, probed at once. */
export async function listDevices(): Promise<DeviceStatus[]> {
  const known = await readKnownDevices();
  return Promise.all(
    known.map(async (tv): Promise<DeviceStatus> => {
      try {
        return { known: tv, state: 'online', device: await probeDevice(tv) };
      } catch (error) {
        const reason = error instanceof Error ? error.message : String(error);
        const hint = error instanceof FixableError ? error.hint : null;
        return { known: tv, state: 'offline', reason, hint };
      }
    }),
  );
}

async function onlyDevice(): Promise<Device> {
  const statuses = await listDevices();
  const online = statuses.flatMap((status) => (status.state === 'online' ? [status.device] : []));
  const [first] = online;
  if (first === undefined) {
    const known =
      statuses.length === 0 ? '' : `None of the ${statuses.length} TVs connected before answers. `;
    throw new FixableError('No TV is connected', `${known}${CONNECT_HINT}`);
  }
  if (online.length > 1) {
    const names = online.map((device) => device.name).join(', ');
    throw new FixableError(
      `${online.length} TVs are connected`,
      `Pick one with --device: ${names}`,
    );
  }
  return first;
}

function findKnown(known: readonly KnownDevice[], target: string): KnownDevice {
  const address = parseDeviceAddress(target);
  const typedPort = target.includes(':');
  const found =
    address === null
      ? known.find((tv) => tv.name === target)
      : known.find((tv) => tv.host === address.host && (!typedPort || tv.port === address.port));
  if (found !== undefined) return found;
  if (address !== null) {
    throw new FixableError(
      `rn-webos is not connected to the TV at ${addressText(address)}`,
      `Turn Key Server on in the TV's Developer Mode app, then run: rn-webos connect ${addressText(address)}`,
    );
  }
  const names = known.map((tv) => tv.name).join(', ');
  throw new FixableError(
    `No connected TV is named ${target}`,
    names === '' ? CONNECT_HINT : `Pass its IP address, or one of: ${names}`,
  );
}

/**
 * The TV a command targets: the connected one with this address or name, put
 * first for next time, or, with neither, the only connected TV that answers.
 */
export async function resolveDevice(target: string | undefined): Promise<Device> {
  if (target === undefined) return onlyDevice();
  const known = findKnown(await readKnownDevices(), target.trim());
  const device = await probeDevice(known);
  await rememberDevice(known);
  return device;
}
