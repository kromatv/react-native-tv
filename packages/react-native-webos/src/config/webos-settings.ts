import { z } from 'zod';

/** The name apps list this package's config plugin under in `expo.plugins`. */
export const WEBOS_PLUGIN = '@kromatv/react-native-webos';

/** Whether a `plugins` entry names this package's config plugin. */
export type PluginMatcher = (name: string) => boolean;

/** The webOS settings an app declares, where they were found, and what to warn about. */
export interface WebOSSettings {
  readonly settings: unknown;
  /** How an issue in them is named, such as `plugins["@kromatv/react-native-webos"]`. */
  readonly where: string;
  readonly deprecation: string | null;
}

const PLUGIN_WHERE = `plugins["${WEBOS_PLUGIN}"]`;
const entrySchema = z.union([z.string(), z.tuple([z.string()]).rest(z.unknown())]);
const pluginsSchema = z.array(z.unknown());

export const EXPO_WEBOS_DEPRECATION = `expo.webos is deprecated: move its fields into the props of the "${WEBOS_PLUGIN}" config plugin in expo.plugins. This release still reads it; the next one will not.`;

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

/**
 * Where an app's webOS settings are: the config plugin's props, else the
 * `expo.webos` of earlier releases, with a deprecation. Both at once is an
 * error, since one would silently win.
 */
export function findWebOSSettings(
  plugins: unknown,
  legacy: unknown,
  isWebOSPlugin: PluginMatcher,
): WebOSSettings | { readonly error: string } {
  const props = webosPluginProps(plugins, isWebOSPlugin);
  if (props !== undefined && legacy !== undefined) {
    return {
      error: `Both expo.webos and ${PLUGIN_WHERE} set the webOS settings: keep the plugin's and remove expo.webos`,
    };
  }
  if (props !== undefined) return { settings: props, where: PLUGIN_WHERE, deprecation: null };
  if (legacy !== undefined)
    return { settings: legacy, where: 'expo.webos', deprecation: EXPO_WEBOS_DEPRECATION };
  return {
    error: `The app has no webOS settings: add ["${WEBOS_PLUGIN}", { "package": "com.example.myapp" }] to expo.plugins`,
  };
}
