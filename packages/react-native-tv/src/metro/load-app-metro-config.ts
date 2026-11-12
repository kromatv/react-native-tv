import { createRequire } from 'node:module';
import { join } from 'node:path';
import { z } from 'zod';
import { tvPlatformPackages } from '../platforms/tv-platform-packages.ts';
import type { MetroConfigLike } from './metro-config-like.ts';
import { withTvPlatforms } from './with-tv.ts';

type LoadUserConfig = (params: {
  projectRoot: string;
  serverRoot: string;
}) => Promise<MetroConfigLike>;

const metroConfigSchema = z.looseObject({
  loadUserConfig: z.custom<LoadUserConfig>((value) => typeof value === 'function'),
});
const pathsSchema = z.looseObject({
  getMetroServerRoot: z.custom<(projectRoot: string) => string>(
    (value) => typeof value === 'function',
  ),
});

/**
 * The Metro config Expo would load for the app at `projectRoot`, its own file
 * or Expo's default, with the TV platforms it does not know yet. Only a
 * package that names its platform is added: for one that does not, nothing
 * tells whether the app's own config wrapped it already.
 */
export async function loadAppMetroConfig(projectRoot: string): Promise<MetroConfigLike> {
  const fromApp = createRequire(join(projectRoot, 'package.json'));
  const fromExpo = createRequire(fromApp.resolve('expo/package.json'));
  const { loadUserConfig } = metroConfigSchema.parse(fromApp('expo/metro-config'));
  const { getMetroServerRoot } = pathsSchema.parse(fromExpo('@expo/config/paths'));
  const config = await loadUserConfig({ projectRoot, serverRoot: getMetroServerRoot(projectRoot) });
  const named = tvPlatformPackages(projectRoot).filter(({ platform }) => platform !== null);
  return withTvPlatforms({ ...config, projectRoot }, named);
}
