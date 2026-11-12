import { FixableError } from '../errors.ts';
import type { DeviceCapabilities } from '../sdb/capabilities.ts';
import { SdbConnectError, type ConnectFailure, type SdbAddress } from '../sdb/sdb-connection.ts';
import { openSdbDevice } from '../sdb/sdb-device.ts';
import { deviceSerial, parseDeviceAddress } from './device-address.ts';
import { readKnownDevices, rememberDevice } from './known-devices.ts';

/** A TV that answered, with what it reported about itself. */
export interface Device {
  readonly address: SdbAddress;
  readonly serial: string;
  readonly name: string;
  readonly capabilities: DeviceCapabilities;
}

/** A remembered TV, probed: online with its details, or why it did not answer. */
export type DeviceStatus =
  | { readonly serial: string; readonly state: 'online'; readonly device: Device }
  | { readonly serial: string; readonly state: 'offline'; readonly reason: string };

export const DEVELOPER_MODE_HINT =
  "On the TV, open Apps, type 12345 on the remote, turn Developer mode on with this computer's IP address, and restart the TV.";

const ONE_CONNECTION_HINT =
  "The TV takes one sdb connection at a time, and another host kept it for the two seconds rn-tizen waited: another rn-tizen on this TV, or Tizen Studio's sdb or Device Manager (`sdb disconnect <tv ip>` frees it). If none is connected, check that Developer mode names this computer's IP.";
const PROBE_TIMEOUT_MS = 5000;

const CONNECT_FAILURES: Record<ConnectFailure, (serial: string) => FixableError> = {
  unreachable: (serial) => new FixableError(`No TV answers at ${serial}`, DEVELOPER_MODE_HINT),
  refused: (serial) => new FixableError(`${serial} refused the connection`, ONE_CONNECTION_HINT),
  silent: (serial) =>
    new FixableError(`${serial} accepted the connection but never answered`, DEVELOPER_MODE_HINT),
};

/** The {@link FixableError} for a TV that could not be reached, or `error` itself. */
export function connectFailure(serial: string, error: unknown): Error {
  if (error instanceof SdbConnectError) return CONNECT_FAILURES[error.reason](serial);
  return error instanceof Error ? error : new Error(String(error));
}

/** Connects to the TV at `address` and reads what it reports. */
export async function probeDevice(
  address: SdbAddress,
  timeoutMs = PROBE_TIMEOUT_MS,
): Promise<Device> {
  const serial = deviceSerial(address);
  const tv = await openSdbDevice(address, timeoutMs).catch((error: unknown) => {
    throw connectFailure(serial, error);
  });
  tv.close();
  if (tv.capabilities === null) {
    throw new FixableError(
      `${serial} did not report its CPU and platform`,
      'Check the TV is on and in Developer mode, then try again.',
    );
  }
  return { address, serial, name: tv.model ?? '', capabilities: tv.capabilities };
}

/** Probes a TV and remembers it, so later commands find it without `--device`. */
export async function connectDevice(address: SdbAddress): Promise<Device> {
  const device = await probeDevice(address);
  await rememberDevice(address);
  return device;
}

/** Every remembered TV, probed at once. */
export async function listDevices(): Promise<DeviceStatus[]> {
  const known = await readKnownDevices();
  return Promise.all(
    known.map(async (address): Promise<DeviceStatus> => {
      const serial = deviceSerial(address);
      try {
        return { serial, state: 'online', device: await probeDevice(address) };
      } catch (error) {
        return {
          serial,
          state: 'offline',
          reason: error instanceof Error ? error.message : String(error),
        };
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
      statuses.length === 0 ? '' : ` None of the ${statuses.length} TVs reached before answered.`;
    throw new FixableError(
      'No TV is connected',
      `Pass the TV's address with --device <ip>.${known} ${DEVELOPER_MODE_HINT}`,
    );
  }
  if (online.length > 1) {
    const serials = online.map((device) => device.serial).join(', ');
    throw new FixableError(
      `${online.length} TVs are connected`,
      `Pick one with --device: ${serials}`,
    );
  }
  return first;
}

/**
 * The TV a command targets: the one at `address`, remembered for next time,
 * or, with no address, the only remembered TV that answers.
 */
export async function resolveDevice(address: string | undefined): Promise<Device> {
  if (address === undefined) return onlyDevice();
  const parsed = parseDeviceAddress(address);
  if (parsed === null) {
    throw new FixableError(
      `${address} is not a TV address`,
      'Pass an IP address, such as --device 192.168.1.50.',
    );
  }
  return connectDevice(parsed);
}
