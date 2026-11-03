import { FixableError } from '../errors.ts';

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

const SHELL_TARGETS: readonly ShellTarget[] = [
  { platformVersion: '9.0', tizenFxApiLevel: 9, tizenNetVersion: '12.0.0.18510' },
];

/** The platforms a package can be built for, oldest first. */
export const SUPPORTED_PLATFORMS: readonly string[] = SHELL_TARGETS.map(
  (target) => target.platformVersion,
);

function compareVersions(left: string, right: string): number {
  const [leftMajor = 0, leftMinor = 0] = left.split('.').map(Number);
  const [rightMajor = 0, rightMinor = 0] = right.split('.').map(Number);
  return leftMajor - rightMajor || leftMinor - rightMinor;
}

/**
 * The profile of a shell target: its target framework comes from its TizenFX
 * API level, which must exist by its platform, and the manifest's
 * `api-version` is the platform version, as `x.y`.
 */
export function profileFor(target: ShellTarget): PlatformProfile {
  const level = TIZENFX_API_LEVELS[target.tizenFxApiLevel];
  if (level === undefined) throw new Error(`No TizenFX API level ${target.tizenFxApiLevel}`);
  if (compareVersions(level.firstPlatform, target.platformVersion) > 0) {
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

/** The shell profile for a platform, or a {@link FixableError} naming the supported ones. */
export function platformProfile(platformVersion: string): PlatformProfile {
  const target = SHELL_TARGETS.find((candidate) => candidate.platformVersion === platformVersion);
  if (target !== undefined) return profileFor(target);
  throw new FixableError(
    `Tizen ${platformVersion} is not a supported platform`,
    `Build for ${SUPPORTED_PLATFORMS.join(' or ')}: pass --platform, or set expo.tizen.platform in app.json.`,
  );
}

/**
 * The newest supported platform a TV running `devicePlatform` can install, since
 * a package runs on its own platform and every later one. Null when the TV is
 * older than every supported platform.
 */
export function newestPlatformFor(devicePlatform: string): string | null {
  const installable = SHELL_TARGETS.filter(
    (target) => compareVersions(target.platformVersion, devicePlatform) <= 0,
  );
  return installable.at(-1)?.platformVersion ?? null;
}

/** True when a TV running `devicePlatform` can install a package built for `platformVersion`. */
export function runsOn(platformVersion: string, devicePlatform: string): boolean {
  return compareVersions(platformVersion, devicePlatform) <= 0;
}
