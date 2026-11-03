import { deviceHost, deviceSerial } from '../tizen/sdb-output.ts';
import { UsageError } from './command.ts';

/** The TV's IP address in what `--device` holds: an address, or an sdb serial such as `192.168.1.50:26101`. */
export function deviceAddress(value: string, command: string): string {
  const host = deviceHost(deviceSerial(value));
  if (host !== null) return host;
  throw new UsageError(`--device must be the TV's IP address, got ${value}`, command);
}
