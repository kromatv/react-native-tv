import { relative, resolve } from 'node:path';
import { loadAppProject, type AppProject } from '../app/app-project.ts';
import { entryPath, type CommandContext } from '../cli/command.ts';
import { chooseLogEndpoint, LOG_OPTIONS } from '../cli/log-options.ts';
import { Reporter } from '../cli/reporter.ts';
import { chooseTarget, TARGET_OPTIONS } from '../cli/target-options.ts';
import type { LogEndpoint } from '../package/runtime-config.ts';
import { platformProfile, type PlatformProfile } from '../tizen/platform-profiles.ts';
import { resolveDevice, type Device } from '../tizen/devices.ts';
import { locateTizenSdk, type TizenSdk } from '../tizen/tizen-sdk.ts';
import type { TizenTarget } from '../tizen/target.ts';

/** The options of every command that builds a package. */
export const BUILD_OPTIONS = {
  device: {
    type: 'string',
    short: 'd',
    placeholder: '<ip>',
    description: 'The TV to build for, whose CPU and Tizen version become the defaults',
  },
  ...TARGET_OPTIONS,
  profile: {
    type: 'string',
    placeholder: '<name>',
    description:
      'The Tizen Studio signing profile (default: expo.tizen.profile, else the active one)',
  },
  author: {
    type: 'string',
    placeholder: '<file.p12>',
    description:
      "The author certificate, instead of the profile's (password in RN_TIZEN_AUTHOR_PASSWORD)",
  },
  distributor: {
    type: 'string',
    placeholder: '<file.p12>',
    description:
      "The distributor certificate, instead of the profile's (password in RN_TIZEN_DISTRIBUTOR_PASSWORD)",
  },
  ...LOG_OPTIONS,
} as const;

interface BuildFlags {
  readonly device?: string;
  readonly arch?: string;
  readonly platform?: string;
  readonly profile?: string;
  readonly author?: string;
  readonly distributor?: string;
  readonly release?: boolean;
  readonly dev?: boolean;
  readonly 'log-host'?: string;
  readonly 'log-port'?: string;
  readonly project?: string;
  readonly entry?: string;
  readonly verbose?: boolean;
}

interface BuildBase {
  readonly reporter: Reporter;
  readonly sdk: TizenSdk;
  readonly app: AppProject;
}

interface PreparedBuild extends BuildBase {
  readonly target: TizenTarget;
  readonly shell: PlatformProfile;
  readonly log: LogEndpoint | null;
}

function describeDevice(device: Device): string {
  const { cpuArch, platformVersion } = device.capabilities;
  return `${device.name || device.serial}, ${cpuArch}, Tizen ${platformVersion}`;
}

/** Reads the app and finds Tizen Studio. */
export function openBuild(flags: BuildFlags, context: CommandContext): BuildBase {
  return {
    reporter: new Reporter({ verbose: flags.verbose ?? false }),
    app: loadAppProject(resolve(context.cwd, flags.project ?? '.'), entryPath(flags, context)),
    sdk: locateTizenSdk(),
  };
}

/** Finds the TV at `address`, or the only one connected, as a step. */
export function findDevice(base: BuildBase, address: string | undefined): Promise<Device> {
  return base.reporter.step('TV', () => resolveDevice(address), describeDevice);
}

/** Settles the target and the log endpoint, and prints what was settled. */
export function settleBuild(
  command: string,
  flags: BuildFlags,
  base: BuildBase,
  device: Device | null,
  cwd: string,
): PreparedBuild {
  const { reporter, app } = base;
  const target = chooseTarget({
    command,
    arch: flags.arch,
    platform: flags.platform,
    configPlatform: app.config.platform,
    device: device?.capabilities,
  });
  const shell = platformProfile(target.platformVersion);
  const log = chooseLogEndpoint({
    command,
    release: flags.release,
    host: flags['log-host'],
    port: flags['log-port'],
    deviceHost: device?.address.host ?? null,
  });
  const where = relative(cwd, app.root);
  const location = where === '' ? '' : `  ${reporter.paint('gray', where)}`;
  reporter.field('App', `${app.config.packageId} ${app.config.version}${location}`);
  reporter.field('Target', `${target.arch}, Tizen ${target.platformVersion}`);
  reporter.field(
    'Build',
    log === null ? 'release' : `development, logs to ${log.host}:${log.port}`,
  );
  return { ...base, target, shell, log };
}
