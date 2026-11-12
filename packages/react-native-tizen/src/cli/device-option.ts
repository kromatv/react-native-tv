import { parseDeviceAddress } from '../tizen/device-address.ts';
import { UsageError } from './command.ts';

/** The TV's IP address in what `--device` holds: an address, or one with its sdb port such as `192.168.1.50:26101`. */
export function deviceAddress(value: string, command: string): string {
  const address = parseDeviceAddress(value);
  if (address !== null) return address.host;
  throw new UsageError(`--device must be the TV's IP address, got ${value}`, command);
}
