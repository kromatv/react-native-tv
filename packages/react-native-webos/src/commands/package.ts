import { statSync } from 'node:fs';
import { isIPv4 } from 'node:net';
import {
  COMMON_OPTIONS,
  ENTRY_OPTIONS,
  parseCommandArgs,
  PROJECT_OPTIONS,
  UsageError,
  type Command,
} from '../cli/command.ts';
import { DEV_OPTION } from '../cli/log-options.ts';
import { formatPath, formatSize } from '../cli/format.ts';
import { packageSteps } from '../pipeline/package-steps.ts';
import { BUILD_OPTIONS, openBuild, settleBuild } from './build-options.ts';

const OPTIONS = {
  ...BUILD_OPTIONS,
  ...DEV_OPTION,
  ...PROJECT_OPTIONS,
  ...ENTRY_OPTIONS,
  ...COMMON_OPTIONS,
};

function deviceHost(device: string | undefined): string | null {
  if (device === undefined) return null;
  if (isIPv4(device)) return device;
  throw new UsageError(`--device must be the TV's IP address, got ${device}`, 'package');
}

/** `rn-webos package`: builds an `.ipk` of the app. */
export const packageCommand: Command = {
  name: 'package',
  summary: 'Build an .ipk of the app, native runtime included',
  options: OPTIONS,
  examples: ['rn-webos package', 'rn-webos package --dev --device 192.168.1.50'],
  async run(args, context) {
    const flags = parseCommandArgs('package', args, OPTIONS);
    const devFlag = flags['log-host'] ?? flags['log-port'] ?? flags['dev-server'];
    if (flags.dev !== true && devFlag !== undefined) {
      throw new UsageError(
        '--log-host, --log-port and --dev-server set up a development build: add --dev',
        'package',
      );
    }
    const host = deviceHost(flags.device);
    const release = { ...flags, release: flags.dev !== true };
    const build = settleBuild('package', release, openBuild(flags, context), host, context.cwd);
    const { reporter } = build;
    const { ipk } = await packageSteps(reporter, {
      app: build.app,
      log: build.log,
      devServer: flags['dev-server'],
    });
    reporter.line();
    const size = formatSize(statSync(ipk).size);
    reporter.line(
      `${reporter.paint(['green', 'bold'], 'Package')} ${formatPath(ipk, context.cwd)} ${reporter.paint('gray', size)}`,
    );
    reporter.note(`Install and launch it with: rn-webos run --device ${host ?? '<tv ip>'}`);
  },
};
