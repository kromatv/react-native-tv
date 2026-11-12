import { z } from 'zod';

/** The name apps list this package's config plugin under in `expo.plugins`. */
export const TIZEN_PLUGIN = '@kromatv/react-native-tizen';

/** Whether a `plugins` entry names this package's config plugin. */
export type PluginMatcher = (name: string) => boolean;

/** The Tizen settings an app declares, where they were found, and what to warn about. */
export interface TizenSettings {
  readonly settings: unknown;
  /** How an issue in them is named, such as `plugins["@kromatv/react-native-tizen"]`. */
  readonly where: string;
  readonly deprecation: string | null;
}

const PLUGIN_WHERE = `plugins["${TIZEN_PLUGIN}"]`;
const entrySchema = z.union([z.string(), z.tuple([z.string()]).rest(z.unknown())]);
const pluginsSchema = z.array(z.unknown());

export const EXPO_TIZEN_DEPRECATION = `expo.tizen is deprecated: move its fields into the props of the "${TIZEN_PLUGIN}" config plugin in expo.plugins. This release still reads it; the next one will not.`;

/** The props given to this package's config plugin, or undefined when it has none or is not listed. */
export function tizenPluginProps(plugins: unknown, isTizenPlugin: PluginMatcher): unknown {
  const entries = pluginsSchema.safeParse(plugins);
  if (!entries.success) return undefined;
  for (const entry of entries.data) {
    const parsed = entrySchema.safeParse(entry);
    if (!parsed.success || typeof parsed.data === 'string') continue;
    const [name, props] = parsed.data;
    if (isTizenPlugin(name)) return props;
  }
  return undefined;
}

/**
 * Where an app's Tizen settings are: the config plugin's props, else the
 * `expo.tizen` of earlier releases, with a deprecation. Both at once is an
 * error, since one would silently win.
 */
export function findTizenSettings(
  plugins: unknown,
  legacy: unknown,
  isTizenPlugin: PluginMatcher,
): TizenSettings | { readonly error: string } {
  const props = tizenPluginProps(plugins, isTizenPlugin);
  if (props !== undefined && legacy !== undefined) {
    return {
      error: `Both expo.tizen and ${PLUGIN_WHERE} set the Tizen settings: keep the plugin's and remove expo.tizen`,
    };
  }
  if (props !== undefined) return { settings: props, where: PLUGIN_WHERE, deprecation: null };
  if (legacy !== undefined)
    return { settings: legacy, where: 'expo.tizen', deprecation: EXPO_TIZEN_DEPRECATION };
  return {
    error: `The app has no Tizen settings: add ["${TIZEN_PLUGIN}", { "package": "com.example.myapp" }] to expo.plugins`,
  };
}
