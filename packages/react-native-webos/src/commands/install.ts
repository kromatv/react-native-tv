import { resolve } from 'node:path';
import { COMMON_OPTIONS, parseCommandArgs, UsageError, type Command } from '../cli/command.ts';
import { Reporter } from '../cli/reporter.ts';
import { splitActionArgs } from '../simulator/action-args.ts';
import { deployPackage, openPackage } from '../tizen/deploy-package.ts';
import { resolveDevice } from '../tizen/devices.ts';
import { resolveSigning } from '../tizen/signing-identity.ts';

const OPTIONS = {
  device: {
    type: 'string',
    short: 'd',
    placeholder: '<ip>',
    description: 'The TV to install on (default: the only one reached before that answers)',
  },
  profile: {
    type: 'string',
    placeholder: '<name>',
    description: 'The Tizen Studio signing profile (default: the active one)',
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
  'keep-signature': {
    type: 'boolean',
    description: 'Install the package as it is signed, instead of signing it for this TV',
  },
  launch: { type: 'boolean', description: 'Launch the app once it is installed' },
  ...COMMON_OPTIONS,
} as const;

/** `rn-tizen install <file>`: signs any `.tpk` or `.wgt` for a TV, installs it, and can launch it. */
export const installCommand: Command = {
  name: 'install',
  summary: 'Sign a .tpk or .wgt for a TV, install it and optionally launch it',
  options: OPTIONS,
  examples: [
    'rn-tizen install app.wgt --device 192.168.1.50 --launch',
    'rn-tizen install build/app.tpk --profile Store',
  ],
  async run(args, context) {
    const { positionals, options } = splitActionArgs(args);
    const flags = parseCommandArgs('install', options, OPTIONS);
    const [file, ...extra] = positionals;
    if (file === undefined || extra.length > 0) {
      throw new UsageError('install needs one package, e.g. `rn-tizen install app.wgt`', 'install');
    }
    const reporter = new Reporter({ verbose: flags.verbose ?? false });
    const opened = await reporter.step(
      'Package',
      () => openPackage(resolve(context.cwd, file)),
      ({ info }) => `${info.appId}, ${info.kind}`,
    );
    const signing =
      flags['keep-signature'] === true
        ? null
        : await reporter.step(
            'Certificates',
            () =>
              resolveSigning({
                profile: flags.profile,
                author: flags.author,
                distributor: flags.distributor,
              }),
            (resolved) => resolved.name,
          );
    const device = await reporter.step(
      'TV',
      () => resolveDevice(flags.device),
      (tv) => tv.name || tv.serial,
    );
    await reporter.step(
      flags.launch === true ? 'Install and launch' : 'Install',
      (task) =>
        deployPackage({
          package: opened,
          device,
          signing,
          launch: flags.launch === true,
          progress: task.progress,
        }),
      () => device.serial,
    );
  },
};
