import { fileURLToPath } from 'node:url';

/** The variable Expo CLI reads the path of a Metro config from, to load in place of the app's own. */
export const EXPO_METRO_OVERRIDE = 'EXPO_OVERRIDE_METRO_CONFIG';

/** The variable that tells this package's Metro config which app it is loaded for. */
export const TV_PROJECT_ROOT = 'RN_TV_PROJECT_ROOT';

/** The Metro config of this package: the app's own, with every TV platform added. */
export const TV_METRO_CONFIG = fileURLToPath(new URL('./tv-metro-config.mjs', import.meta.url));

/**
 * Makes the Expo CLI of this process load this package's Metro config for the
 * app at `projectRoot`, so the app needs no `withTV` call. A Metro config
 * something else already put in Expo's variable is left in place.
 */
export function overrideExpoMetroConfig(
  projectRoot: string,
  env: NodeJS.ProcessEnv = process.env,
): void {
  if (env[EXPO_METRO_OVERRIDE] !== undefined && env[EXPO_METRO_OVERRIDE] !== TV_METRO_CONFIG)
    return;
  env[EXPO_METRO_OVERRIDE] = TV_METRO_CONFIG;
  env[TV_PROJECT_ROOT] = projectRoot;
}
