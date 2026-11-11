import { COMMON_OPTIONS, parseCommandArgs, UsageError, type Command } from '../cli/command.ts';
import { DEVICE_PLACEHOLDER } from '../cli/device-option.ts';
import { Reporter } from '../cli/reporter.ts';
import { splitActionArgs } from '../simulator/action-args.ts';
import { uninstallApp } from '../webos/app-control.ts';
import { addressText } from '../webos/device-address.ts';
import { resolveDevice } from '../webos/devices.ts';

const OPTIONS = {
  device: {
    type: 'string',
    short: 'd',
    placeholder: DEVICE_PLACEHOLDER,
    description: 'The TV to uninstall from (default: the only connected one that answers)',
  },
  ...COMMON_OPTIONS,
} as const;

/** `rn-webos uninstall <app id>`: removes an app and its data from a TV. */
export const uninstallCommand: Command = {
  name: 'uninstall',
  summary: 'Remove an app and its data from a TV',
  options: OPTIONS,
  examples: ['rn-webos uninstall com.example.myapp --device 192.168.1.50'],
  async run(args) {
    const { positionals, options } = splitActionArgs(args, OPTIONS);
    const flags = parseCommandArgs('uninstall', options, OPTIONS);
    const [appId, ...extra] = positionals;
    if (appId === undefined || extra.length > 0) {
      throw new UsageError(
        'uninstall needs one app id, e.g. `rn-webos uninstall com.example.myapp`',
        'uninstall',
      );
    }
    const reporter = new Reporter({ verbose: flags.verbose ?? false });
    const device = await reporter.step(
      'TV',
      () => resolveDevice(flags.device),
      (tv) => tv.name,
    );
    await reporter.step(
      `Uninstall ${appId}`,
      () => uninstallApp(device, appId),
      () => addressText(device.address),
    );
  },
};
