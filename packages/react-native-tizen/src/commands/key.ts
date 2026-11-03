import { COMMON_OPTIONS, parseCommandArgs, UsageError, type Command } from '../cli/command.ts';
import { deviceAddress } from '../cli/device-option.ts';
import { Reporter } from '../cli/reporter.ts';
import { openRemote, type RemoteSession } from '../remote/remote-session.ts';
import { remoteKeyCode } from '../remote/remote-keys.ts';
import { readToken, saveToken } from '../remote/remote-token.ts';
import { splitActionArgs } from '../simulator/action-args.ts';

const PAIRING_TIMEOUT_MS = 30_000;
const CONNECT_TIMEOUT_MS = 10_000;
const KEY_INTERVAL_MS = 300;

const OPTIONS = {
  device: {
    type: 'string',
    short: 'd',
    placeholder: '<ip>',
    description: "The TV's IP address",
  },
  ...COMMON_OPTIONS,
} as const;

function pair(reporter: Reporter, host: string): Promise<RemoteSession> {
  reporter.note(
    `rn-tizen has not paired with the TV at ${host} yet: choose Allow when the TV asks.`,
  );
  return reporter.step(
    'Pair',
    async () => {
      const session = await openRemote({
        host,
        token: null,
        timeoutMs: PAIRING_TIMEOUT_MS,
        keyIntervalMs: KEY_INTERVAL_MS,
      });
      if (session.token !== null) saveToken(host, session.token);
      return session;
    },
    () => `${host}, allowed on the TV`,
  );
}

async function press(
  host: string,
  paired: RemoteSession | null,
  codes: readonly string[],
): Promise<void> {
  const saved = readToken(host);
  const session =
    paired ??
    (await openRemote({
      host,
      token: saved,
      timeoutMs: CONNECT_TIMEOUT_MS,
      keyIntervalMs: KEY_INTERVAL_MS,
    }));
  try {
    if (session.token !== null && session.token !== saved) saveToken(host, session.token);
    await session.press(codes);
  } finally {
    session.close();
  }
}

/** `rn-tizen key`: presses remote keys on a TV through its network remote, in order. */
export const keyCommand: Command = {
  name: 'key',
  summary: "Press remote keys on a TV, in order, through the TV's network remote",
  options: OPTIONS,
  examples: [
    'rn-tizen key Down Down Enter --device 192.168.1.50',
    'rn-tizen key Back --device 192.168.1.50',
    'rn-tizen key 1 --device 192.168.1.50       digits 0 to 9',
    'rn-tizen key KEY_CH_LIST --device 192.168.1.50   any KEY_* code as written',
  ],
  async run(args) {
    const { positionals, options } = splitActionArgs(args);
    const flags = parseCommandArgs('key', options, OPTIONS);
    if (positionals.length === 0) {
      throw new UsageError(
        'key needs the keys to press, e.g. `rn-tizen key Down Enter --device <ip>`',
        'key',
      );
    }
    const codes = positionals.map(remoteKeyCode);
    if (flags.device === undefined)
      throw new UsageError("key needs --device <the TV's IP address>", 'key');
    const host = deviceAddress(flags.device, 'key');
    const reporter = new Reporter({ verbose: flags.verbose ?? false });
    const paired = readToken(host) === null ? await pair(reporter, host) : null;
    await reporter.step(
      'Keys',
      () => press(host, paired, codes),
      () => positionals.join(' '),
    );
  },
};
