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

/** Puts a TV first in the list of those reached. */
export async function rememberDevice(
  address: SdbAddress,
  path = knownDevicesPath(),
): Promise<void> {
  const others = (await readKnownDevices(path)).filter(
    (known) => known.host !== address.host || known.port !== address.port,
  );
  const devices = [{ host: address.host, port: address.port }, ...others].slice(0, MAX_DEVICES);
  await mkdir(dirname(path), { recursive: true });
  await writeFile(path, `${JSON.stringify({ devices }, null, 2)}\n`);
}
