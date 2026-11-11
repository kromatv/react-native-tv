import { UsageError } from '../cli/command.ts';
import { closestMatch } from '../cli/suggest.ts';

const DIGITS = Object.fromEntries(
  Array.from({ length: 10 }, (_, digit) => [String(digit), String(digit)]),
);

const REMOTE_BUTTONS: Readonly<Record<string, string>> = {
  left: 'LEFT',
  right: 'RIGHT',
  up: 'UP',
  down: 'DOWN',
  enter: 'ENTER',
  ok: 'ENTER',
  select: 'ENTER',
  return: 'ENTER',
  back: 'BACK',
  exit: 'EXIT',
  info: 'INFO',
  play: 'PLAY',
  pause: 'PAUSE',
  stop: 'STOP',
  rewind: 'REWIND',
  fastforward: 'FASTFORWARD',
  red: 'RED',
  green: 'GREEN',
  yellow: 'YELLOW',
  blue: 'BLUE',
  ...DIGITS,
};

const RAW_BUTTON = /^[A-Z0-9_]{2,32}$/;

/** The key names `rn-webos key` takes, as `--help` lists them. */
export const REMOTE_KEY_NAMES = Object.keys(REMOTE_BUTTONS);

/**
 * The button name a TV's pointer input socket takes for a key name, in any
 * case: the simulator's names, digits, or a button name in capitals, such as
 * `VOLUMEUP`, passed as written.
 */
export function remoteButton(name: string): string {
  const button = REMOTE_BUTTONS[name.toLowerCase()];
  if (button !== undefined) return button;
  if (name.toLowerCase() === 'playpause') {
    throw new UsageError('An LG TV has no play/pause button to press: send play or pause', 'key');
  }
  if (RAW_BUTTON.test(name)) return name;
  const suggestion = closestMatch(name.toLowerCase(), REMOTE_KEY_NAMES);
  const hint = suggestion === null ? '' : `. Did you mean ${suggestion}?`;
  throw new UsageError(`Unknown key ${name}${hint}`, 'key');
}
