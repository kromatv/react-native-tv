import { z } from 'zod';
import { fetchJson } from './http-text.ts';
import { unescapeName } from './upnp.ts';

/** The port of the REST API Samsung TVs answer on the local network. */
export const SAMSUNG_API_PORT = 8001;

/** What a Samsung TV's REST API says about it. */
export interface SamsungApiDevice {
  readonly name: string;
  readonly modelName: string;
  readonly developerMode: boolean;
  /** The host PC Developer mode names, if any. */
  readonly developerIp: string | null;
}

const short = z.string().max(256);
const apiSchema = z.object({
  device: z.object({
    name: short.optional(),
    model: short.optional(),
    modelName: short.optional(),
    ModelNumber: short.optional(),
    OS: short.optional(),
    type: short.optional(),
    developerMode: z.string().max(8).optional(),
    developerIP: z.string().max(64).optional(),
  }),
});

/** Whether a device is one of Samsung's soundbars, which answer the same API as a TV. */
function isSoundbar(device: z.infer<typeof apiSchema>['device']): boolean {
  return (device.ModelNumber ?? '').startsWith('HW-') || (device.model ?? '').endsWith('SPK');
}

/** Reads `/api/v2/`; null for anything that is not a Tizen TV. */
export function parseApiDevice(json: z.infer<typeof apiSchema>): SamsungApiDevice | null {
  const { device } = json;
  if (device.OS !== 'Tizen' || isSoundbar(device)) return null;
  const developerIp = z.ipv4().safeParse(device.developerIP);
  return {
    name: unescapeName(device.name ?? ''),
    modelName: device.modelName ?? '',
    developerMode: device.developerMode === '1',
    developerIp: developerIp.success && developerIp.data !== '0.0.0.0' ? developerIp.data : null,
  };
}

/** Asks the TV at `host` what it is, or null when nothing Samsung answers there. */
export async function fetchApiDevice(
  host: string,
  port = SAMSUNG_API_PORT,
): Promise<SamsungApiDevice | null> {
  const json = await fetchJson(`http://${host}:${port}/api/v2/`, apiSchema);
  return json === null ? null : parseApiDevice(json);
}
