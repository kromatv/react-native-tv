import { FixableError } from '../errors.ts';
import { compareTizenVersions } from './target.ts';

/** What the .NET shell is built against for one Tizen platform. */
export interface PlatformProfile {
  readonly platformVersion: string;
  readonly tizenFxApiLevel: number;
  readonly targetFramework: string;
  readonly tizenNetVersion: string;
  readonly apiVersion: string;
}

interface TizenFxApiLevel {
  readonly targetFramework: string;
  readonly firstPlatform: string;
}

/**
 * TizenFX API levels and the target framework each is built with, from
 * TizenFX's README (docs/reference/tizen-tv-platform.md, section 2). A target
 * framework is never derived from a platform version: `tizen10.0` is Tizen 7.0.
 * The `net6.0-` and `net8.0-` frameworks also need the Tizen .NET workload.
 */
export const TIZENFX_API_LEVELS: Readonly<Record<number, TizenFxApiLevel>> = {
  8: { targetFramework: 'tizen80', firstPlatform: '6.0' },
  9: { targetFramework: 'tizen90', firstPlatform: '6.5' },
  10: { targetFramework: 'tizen10.0', firstPlatform: '7.0' },
  11: { targetFramework: 'net6.0-tizen8.0', firstPlatform: '8.0' },
  12: { targetFramework: 'net6.0-tizen9.0', firstPlatform: '9.0' },
  13: { targetFramework: 'net8.0-tizen10.0', firstPlatform: '10.0' },
};

/** One platform a package can be built for, and what its shell targets. */
export interface ShellTarget {
  readonly platformVersion: string;
  readonly tizenFxApiLevel: number;
  readonly tizenNetVersion: string;
}

/** The oldest Tizen a package can run on (docs/tizen-versions.md). */
export const MINIMUM_PLATFORM = '6.0';

// The shell calls nothing newer than TizenFX API 8 (Tizen 6.0), so every floor
// compiles it there and only the manifest's api-version and the rootstrap move.
const SHELL_API_LEVEL = 8;
const TIZEN_NET_VERSION = '12.0.0.18510';

/** The platforms a package can name as its floor, oldest first. */
export const SUPPORTED_PLATFORMS: readonly string[] = ['6.0', '6.5', '7.0', '8.0', '9.0', '10.0'];

/**
 * The profile of a shell target: its target framework comes from its TizenFX
 * API level, which must exist by its platform, and the manifest's
 * `api-version` is the platform version, as `x.y`.
 */
export function profileFor(target: ShellTarget): PlatformProfile {
  const level = TIZENFX_API_LEVELS[target.tizenFxApiLevel];
  if (level === undefined) throw new Error(`No TizenFX API level ${target.tizenFxApiLevel}`);
  if (compareTizenVersions(level.firstPlatform, target.platformVersion) > 0) {
    throw new Error(
      `TizenFX API level ${target.tizenFxApiLevel} needs Tizen ${level.firstPlatform}, newer than ${target.platformVersion}`,
    );
  }
  return {
    platformVersion: target.platformVersion,
    tizenFxApiLevel: target.tizenFxApiLevel,
    targetFramework: level.targetFramework,
    tizenNetVersion: target.tizenNetVersion,
    apiVersion: target.platformVersion,
  };
}

/** A {@link FixableError} for a TV or floor below {@link MINIMUM_PLATFORM}. */
export function belowMinimumError(what: string): FixableError {
  return new FixableError(
    `${what} is older than Tizen ${MINIMUM_PLATFORM}, the oldest a native app runs on`,
    'Samsung TVs before 2021 refuse the native libraries an app carries; reach them with a web build (docs/tizen-versions.md).',
  );
}

/** The shell profile for a floor, or a {@link FixableError} naming the supported ones. */
export function platformProfile(platformVersion: string): PlatformProfile {
  if (compareTizenVersions(platformVersion, MINIMUM_PLATFORM) < 0) {
    throw belowMinimumError(`Tizen ${platformVersion}`);
  }
  if (!SUPPORTED_PLATFORMS.includes(platformVersion)) {
    throw new FixableError(
      `Tizen ${platformVersion} is not a supported platform`,
      `Build for ${SUPPORTED_PLATFORMS.join(', ')}: pass --platform, or set the plugin's platform prop.`,
    );
  }
  return profileFor({
    platformVersion,
    tizenFxApiLevel: SHELL_API_LEVEL,
    tizenNetVersion: TIZEN_NET_VERSION,
  });
}

/** True when a TV running `devicePlatform` can install a package built for `platformVersion`. */
export function runsOn(platformVersion: string, devicePlatform: string): boolean {
  return compareTizenVersions(platformVersion, devicePlatform) <= 0;
}
