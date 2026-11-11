import { z } from 'zod';
import type { SdbConnection } from './sdb-connection.ts';

/** What a TV reports through the `capability:` service that a build and an install need. */
export interface DeviceCapabilities {
  readonly cpuArch: string;
  readonly platformVersion: string;
  readonly profile: string | null;
  /** The directory sdb pushes packages under before installing them. */
  readonly toolPath: string | null;
}

const LENGTH_BYTES = 2;
const MAX_PAYLOAD_BYTES = LENGTH_BYTES + 0xffff;
const MAX_ENTRIES = 256;
const SAFE_PATH = /^\/[A-Za-z0-9._/-]+$/;

const capabilitiesSchema = z.object({
  cpu_arch: z.string().min(1).max(64),
  platform_version: z.string().regex(/^\d+\.\d+/),
  profile_name: z.string().max(64).optional(),
  sdk_toolpath: z.string().regex(SAFE_PATH).max(256).optional(),
});

/** The `key:value` lines after the payload's uint16 little-endian byte count. */
export function capabilityEntries(payload: Buffer): Record<string, string> {
  if (payload.length < LENGTH_BYTES) return {};
  const declared = Math.min(payload.readUInt16LE(0), payload.length - LENGTH_BYTES);
  const text = payload.subarray(LENGTH_BYTES, LENGTH_BYTES + declared).toString('utf8');
  const entries: [string, string][] = [];
  for (const line of text.split('\n').slice(0, MAX_ENTRIES)) {
    const separator = line.indexOf(':');
    if (separator > 0)
      entries.push([line.slice(0, separator).trim(), line.slice(separator + 1).trim()]);
  }
  return Object.fromEntries(entries);
}

/** Null when the payload lacks the CPU or the platform version. */
export function parseCapabilities(payload: Buffer): DeviceCapabilities | null {
  const parsed = capabilitiesSchema.safeParse(capabilityEntries(payload));
  if (!parsed.success) return null;
  return {
    cpuArch: parsed.data.cpu_arch,
    platformVersion: parsed.data.platform_version,
    profile: parsed.data.profile_name ?? null,
    toolPath: parsed.data.sdk_toolpath ?? null,
  };
}

export async function readCapabilities(
  connection: SdbConnection,
): Promise<DeviceCapabilities | null> {
  const stream = await connection.openStream('capability:');
  try {
    return parseCapabilities(await stream.drain(MAX_PAYLOAD_BYTES));
  } finally {
    stream.close();
  }
}
