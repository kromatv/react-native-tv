import { COMMON_OPTIONS, parseCommandArgs, UsageError, type Command } from '../cli/command.ts';
import { DEVICE_PLACEHOLDER, deviceHost } from '../cli/device-option.ts';
import { Reporter } from '../cli/reporter.ts';
import { pressRemoteKeys } from '../remote/press-keys.ts';
import { remoteButton } from '../remote/remote-keys.ts';
import { splitActionArgs } from '../simulator/action-args.ts';
import { readKnownDevices } from '../webos/known-devices.ts';

const OPTIONS = {
  device: {
    type: 'string',
    short: 'd',
    placeholder: DEVICE_PLACEHOLDER,
    description: 'The TV to press them on (default: the TV connected last)',
  },
  ...COMMON_OPTIONS,
} as const;

async function targetHost(device: string | undefined): Promise<string> {
  if (device !== undefined) return deviceHost(device, 'key');
  const [last] = await readKnownDevices();
  if (last !== undefined) return last.host;
  throw new UsageError("key needs --device <the TV's IP address>", 'key');
}

/** `rn-webos key`: presses remote keys on a TV through its network remote, in order. */
export const keyCommand: Command = {
  name: 'key',
  summary:
    "Press remote keys on a TV, in order, through the TV's network remote (a press only: it cannot hold a key)",
  options: OPTIONS,
  examples: [
    'rn-webos key Down Down Enter --device 192.168.1.50',
    'rn-webos key Back',
    'rn-webos key 1                digits 0 to 9',
    "rn-webos key VOLUMEUP         any of the TV's button names, in capitals, as written",
  ],
  async run(args) {
    const { positionals, options } = splitActionArgs(args, OPTIONS);
    const flags = parseCommandArgs('key', options, OPTIONS);
    if (positionals.length === 0) {
      throw new UsageError(
        'key needs the keys to press, e.g. `rn-webos key Down Enter --device <ip>`',
        'key',
      );
    }
    positionals.forEach(remoteButton);
    const host = await targetHost(flags.device);
    const reporter = new Reporter({ verbose: flags.verbose ?? false });
    const onPrompt = () =>
      reporter.note(
        `rn-webos has not paired with the TV at ${host} yet: accept the connection request it shows.`,
      );
    await reporter.step(
      'Keys',
      () => pressRemoteKeys(host, positionals, { onPrompt }),
      () => positionals.join(' '),
    );
  },
};
