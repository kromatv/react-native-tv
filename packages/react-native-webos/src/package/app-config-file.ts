import { writeFile } from 'node:fs/promises';
import { join } from 'node:path';
import { readExpoConfig } from '../app/expo-config.ts';

/** The file ExponentConstants reads from a package's `res` directory, named as on Android and iOS. */
export const APP_CONFIG_FILE = 'app.config';

/**
 * Writes the app's public Expo config to `dir/app.config`, as
 * `expo-constants/scripts/getAppConfig.js` does for the other platforms, so
 * `Constants.expoConfig` is the config the app was built from.
 */
export async function writeAppConfig(appRoot: string, dir: string): Promise<void> {
  const exp = readExpoConfig(appRoot, { isPublicConfig: true });
  await writeFile(join(dir, APP_CONFIG_FILE), JSON.stringify(exp));
}
