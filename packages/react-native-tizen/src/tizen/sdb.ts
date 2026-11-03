import { FixableError } from '../errors.ts';
import type { Task } from '../task.ts';
import {
  deviceHost,
  deviceSerial,
  parseCapabilities,
  parseDeviceList,
  type ConnectedDevice,
  type DeviceCapabilities,
} from './sdb-output.ts';
import type { TizenSdk } from './tizen-sdk.ts';

/** A TV sdb can reach, with what it reported about itself. */
export interface Device {
  readonly serial: string;
  readonly name: string;
  readonly capabilities: DeviceCapabilities;
}

const DEVELOPER_MODE_HINT =
  "On the TV, open Apps, type 12345 on the remote, turn Developer mode on with this computer's IP address, and restart the TV.";

/** Every device sdb knows, connected or not. */
export async function listDevices(task: Task, sdk: TizenSdk): Promise<ConnectedDevice[]> {
  return parseDeviceList(await task.run([sdk.sdb, 'devices']));
}

/** `sdb connect`, which sdb reports success or failure of in its output only. */
export async function connectDevice(task: Task, sdk: TizenSdk, serial: string): Promise<void> {
  const output = await task.run([sdk.sdb, 'connect', serial]);
  if (/\bconnected to\b/.test(output) && !/fail|error|unable/i.test(output)) return;
  throw new FixableError(`Could not connect to ${serial}`, DEVELOPER_MODE_HINT);
}

/** What a TV reports through `sdb capability`. */
export async function readCapabilities(
  task: Task,
  sdk: TizenSdk,
  serial: string,
): Promise<DeviceCapabilities> {
  const capabilities = parseCapabilities(await task.run([sdk.sdb, '-s', serial, 'capability']));
  if (capabilities !== null) return capabilities;
  throw new FixableError(
    `${serial} did not report its CPU and platform`,
    'Check the TV is on and in Developer mode, then try again.',
  );
}

async function findOnline(task: Task, sdk: TizenSdk, serial: string) {
  const devices = await listDevices(task, sdk);
  return devices.find((device) => device.serial === serial && device.state === 'device');
}

async function requestedDevice(
  task: Task,
  sdk: TizenSdk,
  address: string,
): Promise<ConnectedDevice> {
  const serial = deviceSerial(address);
  const known = await findOnline(task, sdk, serial);
  if (known !== undefined) return known;
  if (deviceHost(serial) === null) {
    throw new FixableError(`No device ${serial}`, '`rn-tizen devices` lists the TVs sdb sees.');
  }
  await connectDevice(task, sdk, serial);
  const connected = await findOnline(task, sdk, serial);
  if (connected !== undefined) return connected;
  throw new FixableError(`${serial} connected but is not ready`, DEVELOPER_MODE_HINT);
}

async function onlyDevice(task: Task, sdk: TizenSdk): Promise<ConnectedDevice> {
  const online = (await listDevices(task, sdk)).filter((device) => device.state === 'device');
  const [first] = online;
  if (first === undefined) {
    throw new FixableError(
      'No TV is connected',
      `Pass the TV's address with --device <ip>. ${DEVELOPER_MODE_HINT}`,
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
 * The TV a command targets: the one at `address`, connected first if sdb does
 * not know it yet, or the only one connected when no address is given.
 */
export async function resolveDevice(
  task: Task,
  sdk: TizenSdk,
  address: string | undefined,
): Promise<Device> {
  const device =
    address === undefined ? await onlyDevice(task, sdk) : await requestedDevice(task, sdk, address);
  return {
    serial: device.serial,
    name: device.name,
    capabilities: await readCapabilities(task, sdk, device.serial),
  };
}
