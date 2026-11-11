import { statSync } from 'node:fs';
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
import { BUILD_OPTIONS, findDevice, openBuild, settleBuild } from './build-options.ts';

const OPTIONS = {
  ...BUILD_OPTIONS,
  ...DEV_OPTION,
  ...PROJECT_OPTIONS,
  ...ENTRY_OPTIONS,
  ...COMMON_OPTIONS,
};

/** `rn-tizen package`: builds a signed `.tpk` of the app. */
export const packageCommand: Command = {
  name: 'package',
  summary: 'Build a signed .tpk of the app, native runtime included',
  options: OPTIONS,
  examples: [
    'rn-tizen package',
    'rn-tizen package --profile Store',
    'rn-tizen package --dev --device 192.168.1.50',
  ],
  async run(args, context) {
    const flags = parseCommandArgs('package', args, OPTIONS);
    const logFlag = flags['log-host'] ?? flags['log-port'];
    if (flags.dev !== true && logFlag !== undefined) {
      throw new UsageError(
        '--log-host and --log-port set up a development build: add --dev',
        'package',
      );
    }
    const base = openBuild(flags, context);
    const device = flags.device === undefined ? null : await findDevice(base, flags.device);
    const release = { ...flags, release: flags.dev !== true };
    const build = settleBuild('package', release, base, device, context.cwd);
    const { reporter } = build;
    const { tpk } = await packageSteps(reporter, {
      ...build,
      signing: { profile: flags.profile, author: flags.author, distributor: flags.distributor },
      devServer: flags['dev-server'],
    });
    reporter.line();
    const size = formatSize(statSync(tpk).size);
    reporter.line(
      `${reporter.paint(['green', 'bold'], 'Package')} ${formatPath(tpk, context.cwd)} ${reporter.paint('gray', size)}`,
    );
    const target = device === null ? '<tv ip>' : device.serial;
    reporter.note(`Install and launch it with: rn-tizen run --device ${target}`);
  },
};
