import { createRequire } from 'node:module';
import { join } from 'node:path';
import { z } from 'zod';
import { overrideExpoMetroConfig } from '../metro/expo-metro-override.ts';
import { tvPlatformPackages } from '../platforms/tv-platform-packages.ts';
import { listedPlugins } from './listed-plugins.ts';
import { tvPluginEntries, type PluginEntry } from './tv-plugin-entries.ts';
import { tvProps } from './tv-props.ts';

/** The part of an Expo config the plugin reads. */
export interface ExpoConfigLike {
  readonly plugins?: unknown;
  readonly _internal?: unknown;
  readonly [key: string]: unknown;
}

type WithPlugins = <Config>(config: Config, plugins: readonly PluginEntry[]) => Config;

const internalSchema = z.looseObject({ projectRoot: z.string().min(1) });
const configPluginsSchema = z.looseObject({
  withPlugins: z.custom<WithPlugins>((value) => typeof value === 'function'),
});

function projectRootOf(config: ExpoConfigLike): string {
  const internal = internalSchema.safeParse(config._internal);
  return internal.success ? internal.data.projectRoot : process.cwd();
}

function expoWithPlugins(projectRoot: string): WithPlugins {
  const fromApp = createRequire(join(projectRoot, 'package.json'));
  return configPluginsSchema.parse(fromApp('expo/config-plugins')).withPlugins;
}

/**
 * The config plugin `app.plugin.js` exports: one entry in `expo.plugins` for
 * every TV. A shared field (`package`, `version`, `icon`, `locales`) goes to
 * each TV platform package the app has, a block named after a platform
 * goes to that platform, and `tvos` goes to react-native-tvos' plugin, with
 * `isTV` on. It applies those plugins, and adds each platform's entry to the
 * evaluated config, where that platform's own command reads its settings. It
 * also gives the Expo CLI that evaluates it a Metro config with every TV
 * platform, so the app's own needs no `withTV`.
 */
export default function withReactNativeTV<Config extends ExpoConfigLike>(
  config: Config,
  props?: unknown,
): Config {
  const projectRoot = projectRootOf(config);
  overrideExpoMetroConfig(projectRoot);
  const entries = tvPluginEntries(
    projectRoot,
    tvProps(props),
    tvPlatformPackages(projectRoot),
    listedPlugins(config.plugins),
  );
  if (entries.applied.length === 0) return config;
  const configured = expoWithPlugins(projectRoot)(config, entries.applied);
  const listed = z.array(z.unknown()).catch([]).parse(configured.plugins);
  return { ...configured, plugins: [...listed, ...entries.platforms] };
}
