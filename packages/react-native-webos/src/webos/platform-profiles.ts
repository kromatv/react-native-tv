import { FixableError } from '../errors.ts';
import { compareWebOSVersions } from './target.ts';

/** What the .NET shell is built against for one webOS platform. */
export interface PlatformProfile {
  readonly platformVersion: string;
  readonly webosFxApiLevel: number;
  readonly targetFramework: string;
  readonly webosNetVersion: string;
  readonly apiVersion: string;
}

interface WebOSFxApiLevel {
  readonly targetFramework: string;
  readonly firstPlatform: string;
}

/**
 * WebOSFX API levels and the target framework each is built with, from
 * WebOSFX's README (docs/reference/webos-tv-platform.md, section 2). A target
 * framework is never derived from a platform version: `webos10.0` is webOS 7.0.
 * The `net6.0-` and `net8.0-` frameworks also need the webOS .NET workload.
 */
export const WEBOSFX_API_LEVELS: Readonly<Record<number, WebOSFxApiLevel>> = {
  8: { targetFramework: 'webos80', firstPlatform: '6.0' },
  9: { targetFramework: 'webos90', firstPlatform: '6.5' },
  10: { targetFramework: 'webos10.0', firstPlatform: '7.0' },
  11: { targetFramework: 'net6.0-webos8.0', firstPlatform: '8.0' },
  12: { targetFramework: 'net6.0-webos9.0', firstPlatform: '9.0' },
  13: { targetFramework: 'net8.0-webos10.0', firstPlatform: '10.0' },
};

/** One platform a package can be built for, and what its shell targets. */
export interface ShellTarget {
  readonly platformVersion: string;
  readonly webosFxApiLevel: number;
  readonly webosNetVersion: string;
}

/** The oldest webOS a package can run on (docs/webos-versions.md). */
export const MINIMUM_PLATFORM = '6.0';

// The shell calls nothing newer than WebOSFX API 8 (webOS 6.0), so every floor
// compiles it there and only the manifest's api-version and the rootstrap move.
const SHELL_API_LEVEL = 8;
const WEBOS_NET_VERSION = '12.0.0.18510';

/** The platforms a package can name as its floor, oldest first. */
export const SUPPORTED_PLATFORMS: readonly string[] = ['6.0', '6.5', '7.0', '8.0', '9.0', '10.0'];

/**
 * The profile of a shell target: its target framework comes from its WebOSFX
 * API level, which must exist by its platform, and the manifest's
 * `api-version` is the platform version, as `x.y`.
 */
export function profileFor(target: ShellTarget): PlatformProfile {
  const level = WEBOSFX_API_LEVELS[target.webosFxApiLevel];
  if (level === undefined) throw new Error(`No WebOSFX API level ${target.webosFxApiLevel}`);
  if (compareWebOSVersions(level.firstPlatform, target.platformVersion) > 0) {
    throw new Error(
      `WebOSFX API level ${target.webosFxApiLevel} needs webOS ${level.firstPlatform}, newer than ${target.platformVersion}`,
    );
  }
  return {
    platformVersion: target.platformVersion,
    webosFxApiLevel: target.webosFxApiLevel,
    targetFramework: level.targetFramework,
    webosNetVersion: target.webosNetVersion,
    apiVersion: target.platformVersion,
  };
}

/** A {@link FixableError} for a TV or floor below {@link MINIMUM_PLATFORM}. */
export function belowMinimumError(what: string): FixableError {
  return new FixableError(
    `${what} is older than webOS ${MINIMUM_PLATFORM}, the oldest a native app runs on`,
    'Samsung TVs before 2021 refuse the native libraries an app carries; reach them with a web build (docs/webos-versions.md).',
  );
}

/** The shell profile for a floor, or a {@link FixableError} naming the supported ones. */
export function platformProfile(platformVersion: string): PlatformProfile {
  if (compareWebOSVersions(platformVersion, MINIMUM_PLATFORM) < 0) {
    throw belowMinimumError(`webOS ${platformVersion}`);
  }
  if (!SUPPORTED_PLATFORMS.includes(platformVersion)) {
    throw new FixableError(
      `webOS ${platformVersion} is not a supported platform`,
      `Build for ${SUPPORTED_PLATFORMS.join(', ')}: pass --platform, or set the plugin's platform prop.`,
    );
  }
  return profileFor({
    platformVersion,
    webosFxApiLevel: SHELL_API_LEVEL,
    webosNetVersion: WEBOS_NET_VERSION,
  });
}

/** True when a TV running `devicePlatform` can install a package built for `platformVersion`. */
export function runsOn(platformVersion: string, devicePlatform: string): boolean {
  return compareWebOSVersions(platformVersion, devicePlatform) <= 0;
}
