import { mkdir, readFile, writeFile } from 'node:fs/promises';
import { dirname, join } from 'node:path';
import { z } from 'zod';
import type { SdbAddress } from '../sdb/sdb-connection.ts';
import { stateDir } from '../state-dir.ts';

const MAX_DEVICES = 32;
const MAX_FILE_BYTES = 64 * 1024;

const knownDevicesSchema = z.object({
  devices: z
    .array(z.object({ host: z.ipv4(), port: z.number().int().min(1).max(65535) }))
    .max(MAX_DEVICES),
});

/** Where `rn-tizen` remembers the TVs it reached, as sdb's server would. */
export function knownDevicesPath(env: NodeJS.ProcessEnv = process.env): string {
  return join(stateDir(env), 'devices.json');
}

/** The TVs reached before, most recent first. A missing or unreadable file is none. */
export async function readKnownDevices(path = knownDevicesPath()): Promise<SdbAddress[]> {
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

const sameAddress = (left: SdbAddress, right: SdbAddress) =>
  left.host === right.host && left.port === right.port;

/** Puts TVs first in the list of those reached, in the order given. */
export async function rememberDevices(
  addresses: readonly SdbAddress[],
  path = knownDevicesPath(),
): Promise<void> {
  if (addresses.length === 0) return;
  const others = (await readKnownDevices(path)).filter(
    (known) => !addresses.some((address) => sameAddress(address, known)),
  );
  const fresh = addresses.map(({ host, port }) => ({ host, port }));
  const devices = [...fresh, ...others].slice(0, MAX_DEVICES);
  await mkdir(dirname(path), { recursive: true });
  await writeFile(path, `${JSON.stringify({ devices }, null, 2)}\n`);
}

/** Puts a TV first in the list of those reached. */
export function rememberDevice(address: SdbAddress, path = knownDevicesPath()): Promise<void> {
  return rememberDevices([address], path);
}
