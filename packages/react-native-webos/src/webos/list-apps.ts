import { z } from 'zod';
import type { TvAddress } from './device-address.ts';
import { lunaCall } from './luna.ts';
import { onDevice } from './tv-session.ts';

const LIST = 'luna://com.webos.applicationManager/dev/listApps';
const RUNNING = 'luna://com.webos.applicationManager/dev/running';
const MAX_APPS = 512;

const text = z.string().max(1024);
const listSchema = z.object({
  apps: z
    .array(
      z.object({
        id: text,
        version: text.optional(),
        title: text.optional(),
        type: text.optional(),
        visible: z.boolean().optional(),
      }),
    )
    .max(MAX_APPS),
});
const runningSchema = z.object({
  returnValue: z.literal(true),
  running: z.array(z.object({ id: text })).max(MAX_APPS),
});

/** An app installed in Developer Mode. A field the TV did not report is empty. */
export interface InstalledApp {
  readonly id: string;
  readonly version: string;
  readonly title: string;
  /** `native` for an app built by `rn-webos`, `web` for a web app. */
  readonly type: string;
}

type Tv = { readonly address: TvAddress };

/** The apps installed in Developer Mode, without the ones the TV hides from its launcher. */
export function listInstalledApps(device: Tv): Promise<InstalledApp[]> {
  return onDevice(device, async (session) => {
    const { apps } = await lunaCall(session, LIST, { subscribe: false }, listSchema);
    return apps
      .filter((app) => app.visible !== false)
      .map((app) => ({
        id: app.id,
        version: app.version ?? '',
        title: app.title ?? '',
        type: app.type ?? '',
      }));
  });
}

/** The ids of the Developer Mode apps that are running. */
export function listRunningApps(device: Tv): Promise<string[]> {
  return onDevice(device, async (session) => {
    const { running } = await lunaCall(session, RUNNING, { subscribe: false }, runningSchema);
    return running.map((app) => app.id);
  });
}
