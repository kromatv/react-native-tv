import { z } from 'zod';
import { appIdSchema, appVersionSchema } from '../config/app-id.ts';
import type { WebOSAppConfig } from '../config/webos-config.ts';
import { RUNTIME_EXECUTABLE } from '../runtime/build-runtime.ts';

/** The file the TV's app manager reads an app from, at the root of its directory. */
export const APPINFO_FILE = 'appinfo.json';

/** The launcher icon, 80x80. */
export const ICON_FILE = 'icon.png';

/** The 130x130 icon. */
export const LARGE_ICON_FILE = 'largeIcon.png';

/** The image shown while the app loads. */
export const SPLASH_FILE = 'splash.png';

/** What a packager needs back from an `appinfo.json`. */
export const appInfoSchema = z.looseObject({
  id: appIdSchema,
  version: appVersionSchema,
  type: z.string().min(1),
  main: z.string().min(1),
});

/** What of an app, beyond its config, decides fields of its `appinfo.json`. */
export interface AppInfoTraits {
  readonly hasSplash: boolean;
  /**
   * The app can show a video. The TV plays one on a plane under the app's
   * window and shows it only through a window declared `transparent`
   * (docs/reference/webos-media-playback.md, section 2).
   */
  readonly showsVideo: boolean;
}

/**
 * The `appinfo.json` of an app: a native executable that registers with the
 * app manager through `registerApp`, lifecycle interface 2, and asks to be
 * told when it is launched again while it runs. `requiredACG` lists the groups
 * of the Luna calls it makes, which webOS TV 26 Re:New enforces.
 */
export function appInfo(
  config: WebOSAppConfig,
  { hasSplash, showsVideo }: AppInfoTraits,
): Readonly<Record<string, unknown>> {
  return {
    id: config.packageId,
    version: config.version,
    ...(config.vendor === undefined ? {} : { vendor: config.vendor }),
    type: 'native',
    main: RUNTIME_EXECUTABLE,
    title: config.title,
    icon: ICON_FILE,
    largeIcon: LARGE_ICON_FILE,
    ...(config.iconColor === undefined ? {} : { iconColor: config.iconColor }),
    ...(hasSplash ? { splashBackground: SPLASH_FILE } : {}),
    ...(showsVideo ? { transparent: true } : {}),
    nativeLifeCycleInterfaceVersion: 2,
    handlesRelaunch: true,
    requiredACG: config.permissions,
  };
}
