import { z } from 'zod';

/** The name apps list this package's config plugin under in `expo.plugins`. */
export const WEBOS_PLUGIN = '@kromatv/react-native-webos';

/** Whether a `plugins` entry names this package's config plugin. */
export type PluginMatcher = (name: string) => boolean;

/** How an issue in the plugin's props is named. */
export const PLUGIN_WHERE = `plugins["${WEBOS_PLUGIN}"]`;

const entrySchema = z.union([z.string(), z.tuple([z.string()]).rest(z.unknown())]);
const pluginsSchema = z.array(z.unknown());

/** The props given to this package's config plugin, or undefined when it has none or is not listed. */
export function webosPluginProps(plugins: unknown, isWebOSPlugin: PluginMatcher): unknown {
  const entries = pluginsSchema.safeParse(plugins);
  if (!entries.success) return undefined;
  for (const entry of entries.data) {
    const parsed = entrySchema.safeParse(entry);
    if (!parsed.success || typeof parsed.data === 'string') continue;
    const [name, props] = parsed.data;
    if (isWebOSPlugin(name)) return props;
  }
  return undefined;
}

/** What to tell an app whose config lists no webOS settings. */
export const MISSING_SETTINGS = `The app has no webOS settings: add ["${WEBOS_PLUGIN}", { "package": "com.example.myapp" }] to expo.plugins`;
