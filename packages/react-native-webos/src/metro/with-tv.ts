import { loadTvPlatform, tvPlatformPackages, type TvPlatformWrapper } from './tv-platforms.ts';
import { withWebOS, type MetroConfigLike } from './with-webos.ts';

/** This package's name, whose own wrapper {@link withTV} applies without loading it again. */
export const OWN_PACKAGE = '@kromatv/react-native-webos';

/**
 * Teaches a Metro config every TV platform the app depends on, in one call:
 * `webos` from this package, and the platform of each other dependency that
 * declares one, such as `@kromatv/react-native-tizen`. Adding a platform to an
 * app is then installing its package. The platforms are applied in the order
 * of their package names, so the config is the same whichever package
 * `withTV` is imported from.
 */
export function withTV<Config extends MetroConfigLike>(config: Config): Config {
  const projectRoot = config.projectRoot ?? process.cwd();
  const wrappers = new Map<string, TvPlatformWrapper>([[OWN_PACKAGE, withWebOS]]);
  for (const platform of tvPlatformPackages(projectRoot)) {
    if (!wrappers.has(platform.name))
      wrappers.set(platform.name, loadTvPlatform(projectRoot, platform));
  }
  return [...wrappers.entries()]
    .sort(([first], [second]) => first.localeCompare(second))
    .reduce((wrapped, [, wrap]) => wrap(wrapped), config);
}
