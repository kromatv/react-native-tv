import { createRequire } from 'node:module';
import { join } from 'node:path';
import { z } from 'zod';
import type { TvPlatformPackage } from '../platforms/tv-platform-packages.ts';
import type { TvPlatformWrapper } from './metro-config-like.ts';

/** What a TV platform's Metro entry exports for `withTV` to apply. */
export const TV_PLATFORM_EXPORT = 'withTvPlatform';

const entrySchema = z.looseObject({
  [TV_PLATFORM_EXPORT]: z.custom<TvPlatformWrapper>((value) => typeof value === 'function'),
});

/** The wrapper `platform` exports, loaded as the app at `projectRoot` resolves it. */
export function loadTvPlatform(
  projectRoot: string,
  platform: TvPlatformPackage,
): TvPlatformWrapper {
  const entry = entrySchema.safeParse(
    createRequire(join(projectRoot, 'package.json'))(platform.metro),
  );
  if (!entry.success) {
    throw new Error(
      `${platform.name} declares a TV platform, but ${platform.metro} exports no ${TV_PLATFORM_EXPORT} function`,
    );
  }
  return entry.data[TV_PLATFORM_EXPORT];
}
