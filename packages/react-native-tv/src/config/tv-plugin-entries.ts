import { createRequire } from 'node:module';
import { join } from 'node:path';
import type { TvPlatformPackage } from '../platforms/tv-platform-packages.ts';
import { TVOS_BLOCK, TvPropsError, type TvProps } from './tv-props.ts';

/** react-native-tvos' config plugin, which sets an Expo app up for Apple TV and Android TV. */
export const TVOS_PLUGIN = '@react-native-tvos/config-tv';

/** One entry of `expo.plugins`: a plugin's name and its props. */
export type PluginEntry = readonly [name: string, props: Readonly<Record<string, unknown>>];

/** The entries the plugin's props stand for. */
export interface TvPluginEntries {
  /** The entry of each TV platform package, which its own command reads its settings from. */
  readonly platforms: readonly PluginEntry[];
  /** The entry of react-native-tvos' plugin, when the app has it and does not list it itself. */
  readonly tvos: PluginEntry | null;
}

function isInstalled(projectRoot: string, name: string): boolean {
  try {
    createRequire(join(projectRoot, 'package.json')).resolve(`${name}/package.json`);
    return true;
  } catch {
    return false;
  }
}

function tvosEntry(
  projectRoot: string,
  props: TvProps,
  listed: ReadonlySet<string>,
): PluginEntry | null {
  const block = props.blocks.get(TVOS_BLOCK);
  if (listed.has(TVOS_PLUGIN)) {
    if (block === undefined) return null;
    throw new TvPropsError(
      `"${TVOS_BLOCK}" and the app's own ${TVOS_PLUGIN} entry both configure Apple TV and Android TV: keep one`,
    );
  }
  if (isInstalled(projectRoot, TVOS_PLUGIN)) return [TVOS_PLUGIN, { isTV: true, ...block }];
  if (block === undefined) return null;
  throw new TvPropsError(`"${TVOS_BLOCK}" needs ${TVOS_PLUGIN}: install it`);
}

function platformEntry(
  { name, platform }: TvPlatformPackage & { readonly platform: string },
  props: TvProps,
  listed: ReadonlySet<string>,
): PluginEntry | null {
  const block = props.blocks.get(platform);
  if (listed.has(name)) {
    if (block === undefined) return null;
    throw new TvPropsError(
      `"${platform}" and the app's own ${name} entry both configure ${platform}: keep one`,
    );
  }
  if (block === undefined && props.shared.package === undefined) return null;
  return [name, { ...props.shared, ...block }];
}

/**
 * The plugin entries the props stand for, in the app at `projectRoot`: one per
 * TV platform package that names its platform, from the shared fields and the
 * block under that name, and react-native-tvos' plugin with `isTV` on. A
 * platform the app lists itself is left to that entry.
 */
export function tvPluginEntries(
  projectRoot: string,
  props: TvProps,
  packages: readonly TvPlatformPackage[],
  listed: ReadonlySet<string>,
): TvPluginEntries {
  const named = packages.flatMap((entry) =>
    entry.platform === null ? [] : [{ ...entry, platform: entry.platform }],
  );
  const known = new Set([TVOS_BLOCK, ...named.map(({ platform }) => platform)]);
  for (const key of props.blocks.keys()) {
    if (known.has(key)) continue;
    const platforms = [...known].sort((first, second) => first.localeCompare(second)).join(', ');
    throw new TvPropsError(`"${key}" is no TV platform this app depends on. It has ${platforms}`);
  }
  return {
    platforms: named.flatMap((entry) => {
      const listedEntry = platformEntry(entry, props, listed);
      return listedEntry === null ? [] : [listedEntry];
    }),
    tvos: tvosEntry(projectRoot, props, listed),
  };
}
