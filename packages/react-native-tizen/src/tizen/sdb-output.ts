import { z } from 'zod';

/** A device `sdb devices` lists. */
export interface ConnectedDevice {
  readonly serial: string;
  readonly state: string;
  readonly name: string;
}

/** The part of `sdb capability` a build needs. */
export interface DeviceCapabilities {
  readonly cpuArch: string;
  readonly platformVersion: string;
  readonly profile: string | null;
}

const capabilitiesSchema = z.object({
  cpu_arch: z.string().min(1),
  platform_version: z.string().regex(/^\d+\.\d+/),
  profile_name: z.string().optional(),
});

function isDeviceLine(line: string): boolean {
  return line !== '' && !line.startsWith('List of devices') && !line.startsWith('*');
}

/** Parses `sdb devices`. Its header and daemon notices are skipped. */
export function parseDeviceList(output: string): ConnectedDevice[] {
  const devices: ConnectedDevice[] = [];
  for (const line of output.split(/\r?\n/).map((raw) => raw.trim())) {
    if (!isDeviceLine(line)) continue;
    const [serial = '', state = 'unknown', ...name] = line.split(/\s+/);
    devices.push({ serial, state, name: name.join(' ') });
  }
  return devices;
}

/** Parses `sdb capability`'s `key:value` lines; null when the arch or platform is missing. */
export function parseCapabilities(output: string): DeviceCapabilities | null {
  const fields: Record<string, string> = {};
  for (const line of output.split(/\r?\n/)) {
    const separator = line.indexOf(':');
    if (separator > 0) fields[line.slice(0, separator).trim()] = line.slice(separator + 1).trim();
  }
  const parsed = capabilitiesSchema.safeParse(fields);
  if (!parsed.success) return null;
  return {
    cpuArch: parsed.data.cpu_arch,
    platformVersion: parsed.data.platform_version,
    profile: parsed.data.profile_name ?? null,
  };
}

const DEFAULT_SDB_PORT = 26101;

/** An sdb serial for what a user typed: `192.168.1.10` becomes `192.168.1.10:26101`. */
export function deviceSerial(address: string): string {
  const trimmed = address.trim();
  return /^\d{1,3}(\.\d{1,3}){3}$/.test(trimmed) ? `${trimmed}:${DEFAULT_SDB_PORT}` : trimmed;
}

/** The host part of an sdb serial, when the serial is a network address. */
export function deviceHost(serial: string): string | null {
  const match = /^(\d{1,3}(?:\.\d{1,3}){3})(?::\d+)?$/.exec(serial);
  return match?.[1] ?? null;
}
