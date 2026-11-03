import { FixableError } from '../errors.ts';
import { newestPlatformFor, runsOn, SUPPORTED_PLATFORMS } from '../tizen/platform-profiles.ts';
import type { DeviceCapabilities } from '../sdb/capabilities.ts';
import {
  archFromDevice,
  DEFAULT_TARGET,
  platformVersionSchema,
  tizenArchSchema,
  type TizenTarget,
} from '../tizen/target.ts';
import { UsageError, type OptionSpecs } from './command.ts';

/** The options that pick the CPU and platform a build is for. */
export const TARGET_OPTIONS = {
  arch: {
    type: 'string',
    placeholder: '<arch>',
    description: "armv7l, aarch64 or x86_64 (default: the TV's, else armv7l)",
  },
  platform: {
    type: 'string',
    placeholder: '<version>',
    description:
      "The oldest Tizen the package runs on (default: expo.tizen.platform, else the TV's, else 9.0)",
  },
} as const satisfies OptionSpecs;

interface TargetSources {
  readonly command: string;
  readonly arch?: string;
  readonly platform?: string;
  readonly configPlatform?: string;
  readonly device?: Pick<DeviceCapabilities, 'cpuArch' | 'platformVersion'>;
}

function chooseArch(sources: TargetSources) {
  if (sources.arch !== undefined) {
    const parsed = tizenArchSchema.safeParse(sources.arch);
    if (parsed.success) return parsed.data;
    throw new UsageError(
      `--arch must be armv7l, aarch64 or x86_64, got ${sources.arch}`,
      sources.command,
    );
  }
  if (sources.device === undefined) return DEFAULT_TARGET.arch;
  const arch = archFromDevice(sources.device.cpuArch);
  if (arch !== null) return arch;
  throw new FixableError(
    `The TV reports a CPU this tool cannot build for: ${sources.device.cpuArch}`,
    'Pass --arch armv7l, aarch64 or x86_64 if you know which one it is.',
  );
}

function choosePlatform(sources: TargetSources): string {
  if (sources.platform !== undefined) {
    if (platformVersionSchema.safeParse(sources.platform).success) return sources.platform;
    throw new UsageError(`--platform must look like 9.0, got ${sources.platform}`, sources.command);
  }
  if (sources.configPlatform !== undefined) return sources.configPlatform;
  if (sources.device === undefined) return DEFAULT_TARGET.platformVersion;
  const platform = newestPlatformFor(sources.device.platformVersion);
  if (platform !== null) return platform;
  throw new FixableError(
    `The TV runs Tizen ${sources.device.platformVersion}, older than every supported platform`,
    `Supported: Tizen ${SUPPORTED_PLATFORMS.join(', ')} and later.`,
  );
}

/**
 * The target a build is for: flags first, then the app config's platform, then
 * what the TV reports, then armv7l on Tizen 9.0. Refuses a platform newer than
 * the TV's, since that package would not install.
 */
export function chooseTarget(sources: TargetSources): TizenTarget {
  const target = { arch: chooseArch(sources), platformVersion: choosePlatform(sources) };
  const device = sources.device;
  if (device !== undefined && !runsOn(target.platformVersion, device.platformVersion)) {
    throw new FixableError(
      `The TV runs Tizen ${device.platformVersion}, older than the ${target.platformVersion} this build targets`,
      'Pass an older --platform, or lower expo.tizen.platform in app.json.',
    );
  }
  return target;
}
