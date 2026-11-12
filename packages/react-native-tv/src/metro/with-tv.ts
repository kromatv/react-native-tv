import { z } from 'zod';
import { tvPlatformPackages, type TvPlatformPackage } from '../platforms/tv-platform-packages.ts';
import { loadTvPlatform } from './load-tv-platform.ts';
import type { MetroConfigLike } from './metro-config-like.ts';

const knownSchema = z.looseObject({
  resolver: z.looseObject({ platforms: z.array(z.string()).optional() }).optional(),
});

function knownPlatforms(config: MetroConfigLike): Set<string> {
  const parsed = knownSchema.safeParse(config);
  return new Set(parsed.success ? parsed.data.resolver?.platforms : []);
}

/**
 * `config` with the platform of each of `packages` it does not know yet,
 * applied in the order given. A platform already among the config's
 * `resolver.platforms` is left alone, so wrapping a config twice changes
 * nothing the second time.
 */
export function withTvPlatforms<Config extends MetroConfigLike>(
  config: Config,
  packages: readonly TvPlatformPackage[],
): Config {
  const projectRoot = config.projectRoot ?? process.cwd();
  const known = knownPlatforms(config);
  return packages
    .filter(({ platform }) => platform === null || !known.has(platform))
    .map((each) => loadTvPlatform(projectRoot, each))
    .reduce((wrapped, wrap) => wrap(wrapped), config);
}

/**
 * Teaches a Metro config every TV platform the app depends on, in one call:
 * the platform of each dependency that declares one, such as
 * `@kromatv/react-native-tizen` and `@kromatv/react-native-webos`. Adding a
 * platform to an app is then installing its package. The platforms are applied
 * in the order of their package names. An app that lists this package's config
 * plugin needs no call: Expo is given a Metro config that makes it.
 */
export function withTV<Config extends MetroConfigLike>(config: Config): Config {
  return withTvPlatforms(config, tvPlatformPackages(config.projectRoot ?? process.cwd()));
}
