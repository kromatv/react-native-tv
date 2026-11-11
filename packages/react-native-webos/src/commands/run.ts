import {
  COMMON_OPTIONS,
  parseCommandArgs,
  ENTRY_OPTIONS,
  PROJECT_OPTIONS,
  type Command,
  type OptionSpecs,
} from '../cli/command.ts';
import { DEVICE_PLACEHOLDER } from '../cli/device-option.ts';
import { LEVEL_OPTIONS, parseLevel, untilInterrupted } from '../cli/log-level-option.ts';
import { RELEASE_OPTION } from '../cli/log-options.ts';
import { webosVersionName } from '../discovery/webos-version.ts';
import { crashSymbolizer } from '../logs/llvm-symbolizer.ts';
import { LogPrinter } from '../logs/log-printer.ts';
import { listenForLogs, type LogServer } from '../logs/log-server.ts';
import { stagedExecutable } from '../package/stage-app.ts';
import { packageSteps } from '../pipeline/package-steps.ts';
import { launchOn } from '../webos/app-control.ts';
import { addressText } from '../webos/device-address.ts';
import { resolveDevice, type Device } from '../webos/devices.ts';
import { installOn } from '../webos/install-app.ts';
import { onDevice } from '../webos/tv-session.ts';
import { BUILD_OPTIONS, openBuild, settleBuild } from './build-options.ts';

const OPTIONS = {
  ...BUILD_OPTIONS,
  device: {
    type: 'string',
    short: 'd',
    placeholder: DEVICE_PLACEHOLDER,
    description: 'The TV to run on (default: the only connected one that answers)',
  },
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

function describeDevice(device: Device): string {
  const { modelName, sdkVersion } = device.info;
  return [
    device.name,
    modelName === device.name ? '' : modelName,
    sdkVersion === '' ? '' : webosVersionName(sdkVersion),
  ]
    .filter((part) => part !== '')
    .join(', ');
}

/** `rn-webos run`: packages the app, installs it on a TV, launches it and prints its logs. */
export const runCommand: Command = {
  name: 'run',
  summary: 'Package the app, install it on a TV, launch it and print its logs',
  options: OPTIONS,
  examples: [
    'rn-webos run --device 192.168.1.50',
    'rn-webos run',
    'rn-webos run --level debug',
    'rn-webos run --no-logs',
  ],
  async run(args, context) {
    const flags = parseCommandArgs('run', args, OPTIONS);
    const minimum = parseLevel(flags.level, 'run');
    const base = openBuild(flags, context);
    const device = await base.reporter.step(
      'TV',
      () => resolveDevice(flags.device),
      describeDevice,
    );
    const peer = device.address.host;
    const { reporter, app, log } = settleBuild('run', flags, base, peer, context.cwd);
    const printer = new LogPrinter({
      paint: reporter.paint,
      minimum,
      held: true,
      transform: crashSymbolizer(stagedExecutable(app.webosDir)),
    });
    const listening = log !== null && flags['no-logs'] !== true;
    const server: LogServer | null = listening
      ? await listenForLogs(log.port, printer, { peer, app: app.config.packageId })
      : null;
    try {
      const { ipk, appId } = await packageSteps(reporter, {
        app,
        log,
        devServer: flags['dev-server'],
      });
      await onDevice(device, async (session) => {
        await reporter.step(
          'Install',
          (task) => installOn(session, device, ipk, task.progress),
          () => addressText(device.address),
        );
        await reporter.step('Launch', () => launchOn(session, device, appId));
      });
      if (server === null) return;
      reporter.line();
      reporter.note(`Logs from ${appId}. Press Ctrl+C to stop.`);
      printer.release();
      await untilInterrupted();
    } finally {
      await server?.close();
    }
  },
};
