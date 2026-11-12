import { parseDeviceAddress } from '../webos/device-address.ts';
import { readKnownDevices } from '../webos/known-devices.ts';
import { UsageError } from './command.ts';

/** What `--help` says `--device` takes, after the command's own words. */
export const DEVICE_PLACEHOLDER = '<ip or name>';

/** The TV's IP address in what `--device` holds: an address, or the name a TV was connected under. */
export async function deviceHost(value: string, command: string): Promise<string> {
  const address = parseDeviceAddress(value);
  if (address !== null) return address.host;
  const known = (await readKnownDevices()).find((tv) => tv.name === value);
  if (known !== undefined) return known.host;
  throw new UsageError(
    `--device must be the TV's IP address or the name it was connected under, got ${value}`,
    command,
  );
}
