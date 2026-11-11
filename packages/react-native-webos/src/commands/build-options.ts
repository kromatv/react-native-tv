import { relative, resolve } from 'node:path';
import { loadAppProject, type AppProject } from '../app/app-project.ts';
import { entryPath, type CommandContext } from '../cli/command.ts';
import { chooseLogEndpoint, LOG_OPTIONS } from '../cli/log-options.ts';
import { Reporter } from '../cli/reporter.ts';
import { chooseTarget, TARGET_OPTIONS } from '../cli/target-options.ts';
import type { LogEndpoint } from '../package/runtime-config.ts';
import { platformProfile, type PlatformProfile } from '../webos/platform-profiles.ts';
import { resolveDevice, type Device } from '../webos/devices.ts';
import { locateWebOSSdk, type WebOSSdk } from '../webos/webos-sdk.ts';
import type { WebOSTarget } from '../webos/target.ts';

/** The options of every command that builds a package. */
export const BUILD_OPTIONS = {
  device: {
    type: 'string',
    short: 'd',
    placeholder: '<ip>',
    description: 'The TV to build for, whose CPU and webOS version become the defaults',
  },
  ...TARGET_OPTIONS,
  profile: {
    type: 'string',
    placeholder: '<name>',
    description:
      'The webOS Studio signing profile (default: expo.webos.profile, else the active one)',
  },
  author: {
    type: 'string',
    placeholder: '<file.p12>',
    description:
      "The author certificate, instead of the profile's (password in RN_WEBOS_AUTHOR_PASSWORD)",
  },
  distributor: {
    type: 'string',
    placeholder: '<file.p12>',
    description:
      "The distributor certificate, instead of the profile's (password in RN_WEBOS_DISTRIBUTOR_PASSWORD)",
  },
  ...LOG_OPTIONS,
  'dev-server': {
    type: 'string',
    placeholder: '<origin>',
    description:
      'Load the bundle from this `expo start` dev server, such as http://192.168.1.20:8081, instead of packaging it',
  },
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
  readonly 'dev-server'?: string;
  readonly project?: string;
  readonly entry?: string;
  readonly verbose?: boolean;
}

interface BuildBase {
  readonly reporter: Reporter;
  readonly sdk: WebOSSdk;
  readonly app: AppProject;
}

interface PreparedBuild extends BuildBase {
  readonly target: WebOSTarget;
  readonly shell: PlatformProfile;
  readonly log: LogEndpoint | null;
}

function describeDevice(device: Device): string {
  const { cpuArch, platformVersion } = device.capabilities;
  return `${device.name || device.serial}, ${cpuArch}, webOS ${platformVersion}`;
}

/** Reads the app and finds webOS Studio, warning about settings written the deprecated way. */
export function openBuild(flags: BuildFlags, context: CommandContext): BuildBase {
  const reporter = new Reporter({ verbose: flags.verbose ?? false });
  const app = loadAppProject(resolve(context.cwd, flags.project ?? '.'), entryPath(flags, context));
  if (app.config.deprecation !== null)
    reporter.line(reporter.paint('yellow', app.config.deprecation));
  return { reporter, app, sdk: locateWebOSSdk() };
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
  reporter.field('Target', `${target.arch}, webOS ${target.platformVersion}`);
  reporter.field(
    'Build',
    log === null ? 'release' : `development, logs to ${log.host}:${log.port}`,
  );
  return { ...base, target, shell, log };
}
