import { basename, resolve } from 'node:path';
import { COMMON_OPTIONS, parseCommandArgs, UsageError, type Command } from '../cli/command.ts';
import { DEVICE_PLACEHOLDER } from '../cli/device-option.ts';
import { Reporter } from '../cli/reporter.ts';
import { FixableError } from '../errors.ts';
import { splitActionArgs } from '../simulator/action-args.ts';
import { launchOn } from '../webos/app-control.ts';
import { addressText } from '../webos/device-address.ts';
import { resolveDevice } from '../webos/devices.ts';
import { installOn } from '../webos/install-app.ts';
import { onDevice } from '../webos/tv-session.ts';

const OPTIONS = {
  device: {
    type: 'string',
    short: 'd',
    placeholder: DEVICE_PLACEHOLDER,
    description: 'The TV to install on (default: the only connected one that answers)',
  },
  launch: { type: 'boolean', description: 'Launch the app once it is installed' },
  ...COMMON_OPTIONS,
} as const;

// LG's packager, and this one, name a package <app id>_<version>_<architecture>.ipk.
const PACKAGE_FILE = /^([a-z0-9.+-]+)_\d+\.\d+\.\d+_[a-z0-9_]+\.ipk$/;

function appIdOf(installed: string | null, file: string): string {
  const id = installed ?? PACKAGE_FILE.exec(basename(file))?.[1];
  if (id !== undefined) return id;
  throw new FixableError(
    'The package is installed, but the TV did not say which app it holds',
    'Launch it from the TV, or name the file <app id>_<version>_<architecture>.ipk.',
  );
}

/** `rn-webos install <file>`: installs any `.ipk` on a TV, and can launch it. */
export const installCommand: Command = {
  name: 'install',
  summary: 'Install an .ipk on a TV and optionally launch it',
  options: OPTIONS,
  examples: [
    'rn-webos install com.example.myapp_1.0.0_arm.ipk --device 192.168.1.50 --launch',
    'rn-webos install build/app.ipk',
  ],
  async run(args, context) {
    const { positionals, options } = splitActionArgs(args);
    const flags = parseCommandArgs('install', options, OPTIONS);
    const [file, ...extra] = positionals;
    if (file === undefined || extra.length > 0) {
      throw new UsageError('install needs one package, e.g. `rn-webos install app.ipk`', 'install');
    }
    const path = resolve(context.cwd, file);
    const reporter = new Reporter({ verbose: flags.verbose ?? false });
    const device = await reporter.step(
      'TV',
      () => resolveDevice(flags.device),
      (tv) => tv.name,
    );
    await onDevice(device, async (session) => {
      const installed = await reporter.step(
        'Install',
        (task) => installOn(session, device, path, task.progress),
        () => addressText(device.address),
      );
      if (flags.launch !== true) return;
      const appId = appIdOf(installed, path);
      await reporter.step(
        'Launch',
        () => launchOn(session, device, appId),
        () => appId,
      );
    });
  },
};
