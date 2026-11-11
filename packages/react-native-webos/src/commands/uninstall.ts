import { COMMON_OPTIONS, parseCommandArgs, UsageError, type Command } from '../cli/command.ts';
import { Reporter } from '../cli/reporter.ts';
import { splitActionArgs } from '../simulator/action-args.ts';
import { resolveDevice } from '../tizen/devices.ts';
import { uninstallApp } from '../tizen/install-app.ts';

const OPTIONS = {
  device: {
    type: 'string',
    short: 'd',
    placeholder: '<ip>',
    description: 'The TV to uninstall from (default: the only one reached before that answers)',
  },
  ...COMMON_OPTIONS,
} as const;

/** `rn-tizen uninstall <package id>`: removes a package and its data from a TV. */
export const uninstallCommand: Command = {
  name: 'uninstall',
  summary: 'Remove a package and its data from a TV',
  options: OPTIONS,
  examples: ['rn-tizen uninstall com.example.myapp --device 192.168.1.50'],
  async run(args) {
    const { positionals, options } = splitActionArgs(args);
    const flags = parseCommandArgs('uninstall', options, OPTIONS);
    const [packageId, ...extra] = positionals;
    if (packageId === undefined || extra.length > 0) {
      throw new UsageError(
        'uninstall needs one package id, e.g. `rn-tizen uninstall com.example.myapp`',
        'uninstall',
      );
    }
    const reporter = new Reporter({ verbose: flags.verbose ?? false });
    const device = await reporter.step(
      'TV',
      () => resolveDevice(flags.device),
      (tv) => tv.name || tv.serial,
    );
    await reporter.step(
      `Uninstall ${packageId}`,
      () => uninstallApp(device, packageId),
      () => device.serial,
    );
  },
};
