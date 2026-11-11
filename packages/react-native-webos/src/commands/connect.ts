import { ask, canAsk } from '../cli/ask.ts';
import { COMMON_OPTIONS, parseCommandArgs, UsageError, type Command } from '../cli/command.ts';
import { Reporter } from '../cli/reporter.ts';
import { webosVersionName } from '../discovery/webos-version.ts';
import { FixableError } from '../errors.ts';
import { splitActionArgs } from '../simulator/action-args.ts';
import { registerTv } from '../webos/connect-tv.ts';
import { parseDeviceAddress } from '../webos/device-address.ts';
import type { Device } from '../webos/devices.ts';
import { decryptDeveloperKey, fetchDeveloperKey } from '../webos/key-server.ts';

const OPTIONS = {
  passphrase: {
    type: 'string',
    placeholder: '<code>',
    description:
      'The six characters the Developer Mode app shows (default: asked for on a terminal)',
  },
  name: {
    type: 'string',
    placeholder: '<name>',
    description: "What --device calls this TV from now on (default: the TV's model)",
  },
  ...COMMON_OPTIONS,
} as const;

function describe(device: Device): string {
  const { modelName, sdkVersion } = device.info;
  const version = sdkVersion === '' ? '' : webosVersionName(sdkVersion);
  return [device.name, modelName === device.name ? '' : modelName, version]
    .filter((part) => part !== '')
    .join(', ');
}

async function passphrase(flag: string | undefined): Promise<string> {
  if (flag !== undefined) return flag;
  if (!canAsk()) {
    throw new UsageError(
      'connect needs --passphrase <the six characters the Developer Mode app shows>',
      'connect',
    );
  }
  return ask('Passphrase shown in the Developer Mode app', '');
}

/** `rn-webos connect <ip>`: fetches a TV's SSH key from its key server and remembers the TV. */
export const connectCommand: Command = {
  name: 'connect',
  summary: 'Set a TV in Developer Mode up for run and install: fetch its key, once',
  options: OPTIONS,
  examples: [
    'rn-webos connect 192.168.1.50',
    'rn-webos connect 192.168.1.50 --passphrase A1B2C3 --name living-room',
  ],
  async run(args) {
    const { positionals, options } = splitActionArgs(args);
    const flags = parseCommandArgs('connect', options, OPTIONS);
    const [target, ...extra] = positionals;
    if (target === undefined || extra.length > 0) {
      throw new UsageError(
        "connect needs the TV's IP address, e.g. `rn-webos connect 192.168.1.50`",
        'connect',
      );
    }
    const address = parseDeviceAddress(target);
    if (address === null) {
      throw new FixableError(
        `${target} is not a TV address`,
        'Pass an IP address, such as 192.168.1.50. The Developer Mode app shows it.',
      );
    }
    const reporter = new Reporter({ verbose: flags.verbose ?? false });
    const protectedKey = await reporter.step(
      'Key',
      () => fetchDeveloperKey(address.host),
      () => `from the key server of ${address.host}`,
    );
    const phrase = await passphrase(flags.passphrase);
    const device = await reporter.step(
      'Login',
      () =>
        registerTv({
          address,
          privateKey: decryptDeveloperKey(protectedKey, phrase),
          name: flags.name,
        }),
      describe,
    );
    reporter.note(
      `Connected. Other commands find it as --device ${device.name}, or without --device while it is the only TV that answers.`,
    );
    reporter.note('Key Server can be turned off again on the TV.');
  },
};
