import { tvPlatformPackages } from '../platforms/tv-platform-packages.ts';
import { loadTvPlatform } from './load-tv-platform.ts';
import type { MetroConfigLike } from './metro-config-like.ts';

/**
 * Teaches a Metro config every TV platform the app depends on, in one call:
 * the platform of each dependency that declares one, such as
 * `@kromatv/react-native-tizen` and `@kromatv/react-native-webos`. Adding a
 * platform to an app is then installing its package. The platforms are applied
 * in the order of their package names.
 */
export function withTV<Config extends MetroConfigLike>(config: Config): Config {
  const projectRoot = config.projectRoot ?? process.cwd();
  return tvPlatformPackages(projectRoot)
    .map((platform) => loadTvPlatform(projectRoot, platform))
    .reduce((wrapped, wrap) => wrap(wrapped), config);
}
