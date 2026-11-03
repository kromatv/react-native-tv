import { join } from 'node:path';
import {
  COMMON_OPTIONS,
  parseCommandArgs,
  ENTRY_OPTIONS,
  PROJECT_OPTIONS,
  type Command,
  type OptionSpecs,
} from '../cli/command.ts';
import { LEVEL_OPTIONS, parseLevel, untilInterrupted } from '../cli/log-level-option.ts';
import { RELEASE_OPTION } from '../cli/log-options.ts';
import { crashSymbolizer } from '../logs/llvm-symbolizer.ts';
import { LogPrinter } from '../logs/log-printer.ts';
import { listenForLogs, type LogServer } from '../logs/log-server.ts';
import { packageSteps } from '../pipeline/package-steps.ts';
import { RUNTIME_LIBRARY } from '../runtime/build-runtime.ts';
import { deviceHost } from '../tizen/sdb-output.ts';
import { installPackage, launchPackage } from '../tizen/tizen-cli.ts';
import { BUILD_OPTIONS, findDevice, openBuild, settleBuild } from './build-options.ts';

const OPTIONS = {
  ...BUILD_OPTIONS,
  ...RELEASE_OPTION,
  ...LEVEL_OPTIONS,
  'no-logs': {
    type: 'boolean',
    description: 'Exit once the app is launched instead of printing its logs',
  },
  ...PROJECT_OPTIONS,
  ...ENTRY_OPTIONS,
  ...COMMON_OPTIONS,
} as const satisfies OptionSpecs;

/** `rn-tizen run`: packages the app, installs it on a TV, launches it and prints its logs. */
export const runCommand: Command = {
  name: 'run',
  summary: 'Package the app, install it on a TV, launch it and print its logs',
  options: OPTIONS,
  examples: [
    'rn-tizen run --device 192.168.1.50',
    'rn-tizen run',
    'rn-tizen run --level debug',
    'rn-tizen run --no-logs',
  ],
  async run(args, context) {
    const flags = parseCommandArgs('run', args, OPTIONS);
    const minimum = parseLevel(flags.level, 'run');
    const base = openBuild(flags, context);
    const device = await findDevice(base, flags.device);
    const build = settleBuild('run', flags, base, device, context.cwd);
    const { reporter, sdk, app, log } = build;
    const printer = new LogPrinter({
      paint: reporter.paint,
      minimum,
      held: true,
      transform: crashSymbolizer(join(app.tizenDir, 'lib', RUNTIME_LIBRARY)),
    });
    const listening = log !== null && flags['no-logs'] !== true;
    const peer = deviceHost(device.serial);
    const server: LogServer | null = listening
      ? await listenForLogs(log.port, printer, { peer })
      : null;
    try {
      const tpk = await packageSteps(reporter, { ...build, profile: flags.profile });
      await reporter.step(
        'Install',
        (task) => installPackage(task, sdk, device.serial, tpk),
        () => device.serial,
      );
      await reporter.step('Launch', (task) =>
        launchPackage(task, sdk, device.serial, app.config.packageId),
      );
      if (server === null) return;
      reporter.line();
      reporter.note(`Logs from ${app.config.packageId}. Press Ctrl+C to stop.`);
      printer.release();
      await untilInterrupted();
    } finally {
      await server?.close();
    }
  },
};
