import { mkdir, readFile, writeFile } from 'node:fs/promises';
import { dirname, join } from 'node:path';
import { z } from 'zod';
import { stateDir } from '../state-dir.ts';
import { parseDeviceAddress, type TvAddress } from './device-address.ts';

const MAX_DEVICES = 32;
const MAX_FILE_BYTES = 64 * 1024;
const NAME = /^[A-Za-z0-9][A-Za-z0-9_.-]{0,63}$/;

/** A TV `rn-webos connect` set up: the name it goes by and where it answers. */
export interface KnownDevice extends TvAddress {
  readonly name: string;
}

const knownDevicesSchema = z.object({
  devices: z
    .array(
      z.object({
        name: z.string().regex(NAME),
        host: z.ipv4(),
        port: z.number().int().min(1).max(65535),
      }),
    )
    .max(MAX_DEVICES),
});

/** Whether `name` can name a TV: letters, digits, `.`, `_` and `-`, and not an address. */
export function isDeviceName(name: string): boolean {
  return NAME.test(name) && parseDeviceAddress(name) === null;
}

/** Where `rn-webos` remembers the TVs it was connected to. */
export function knownDevicesPath(env: NodeJS.ProcessEnv = process.env): string {
  return join(stateDir(env), 'devices.json');
}

/** The TVs connected before, most recent first. A missing or unreadable file is none. */
export async function readKnownDevices(path = knownDevicesPath()): Promise<KnownDevice[]> {
  let text: string;
  try {
    text = await readFile(path, 'utf8');
  } catch {
    return [];
  }
  if (text.length > MAX_FILE_BYTES) return [];
  let json: unknown;
  try {
    json = JSON.parse(text);
  } catch {
    return [];
  }
  const parsed = knownDevicesSchema.safeParse(json);
  return parsed.success ? parsed.data.devices : [];
}

export const sameAddress = (left: TvAddress, right: TvAddress) =>
  left.host === right.host && left.port === right.port;

/** Puts a TV first in the list of those connected, under the name given. */
export async function rememberDevice(
  device: KnownDevice,
  path = knownDevicesPath(),
): Promise<void> {
  const others = (await readKnownDevices(path)).filter((known) => !sameAddress(device, known));
  const { name, host, port } = device;
  const devices = [{ name, host, port }, ...others].slice(0, MAX_DEVICES);
  await mkdir(dirname(path), { recursive: true });
  await writeFile(path, `${JSON.stringify({ devices }, null, 2)}\n`);
}
