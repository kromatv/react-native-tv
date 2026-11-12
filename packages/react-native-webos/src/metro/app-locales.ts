import { createRequire } from 'node:module';
import { join } from 'node:path';
import { z } from 'zod';
import { isWebOSPluginEntry } from '../config/plugin-entry.ts';
import { resolveWebOSConfig } from '../config/webos-config.ts';

const configModuleSchema = z.object({
  getConfig: z.custom<(root: string, options: object) => { exp: unknown }>(
    (value) => typeof value === 'function',
  ),
});

/**
 * The locales an app's webOS settings name, read through Expo's own config
 * loader; null when the app has no valid webOS settings, which leaves the
 * default to the transformer.
 */
export function appIntlLocales(projectRoot: string): readonly string[] | null {
  try {
    const requireFromApp = createRequire(join(projectRoot, 'package.json'));
    const { getConfig } = configModuleSchema.parse(requireFromApp('expo/config'));
    const { exp } = getConfig(projectRoot, { skipSDKVersionRequirement: true });
    return resolveWebOSConfig(exp, (name) => isWebOSPluginEntry(projectRoot, name)).locales;
  } catch {
    return null;
  }
}
