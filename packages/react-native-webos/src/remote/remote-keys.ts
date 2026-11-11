import { UsageError } from '../cli/command.ts';
import { closestMatch } from '../cli/suggest.ts';

const DIGITS = Object.fromEntries(
  Array.from({ length: 10 }, (_, digit) => [String(digit), `KEY_${digit}`]),
);

const REMOTE_KEYS: Readonly<Record<string, string>> = {
  left: 'KEY_LEFT',
  right: 'KEY_RIGHT',
  up: 'KEY_UP',
  down: 'KEY_DOWN',
  enter: 'KEY_ENTER',
  ok: 'KEY_ENTER',
  select: 'KEY_ENTER',
  return: 'KEY_ENTER',
  back: 'KEY_RETURN',
  exit: 'KEY_EXIT',
  info: 'KEY_INFO',
  play: 'KEY_PLAY',
  pause: 'KEY_PAUSE',
  playpause: 'KEY_PLAY_BACK',
  stop: 'KEY_STOP',
  rewind: 'KEY_REWIND',
  fastforward: 'KEY_FF',
  red: 'KEY_RED',
  green: 'KEY_GREEN',
  yellow: 'KEY_YELLOW',
  blue: 'KEY_CYAN',
  ...DIGITS,
};

const RAW_KEY = /^KEY_[A-Z0-9_]+$/;

/** The key names `rn-tizen key` takes, as `--help` lists them. */
export const REMOTE_KEY_NAMES = Object.keys(REMOTE_KEYS);

/**
 * The network remote's `KEY_*` code for a key name, in any case: the
 * simulator's names, digits, or a `KEY_*` code passed as written.
 */
export function remoteKeyCode(name: string): string {
  if (RAW_KEY.test(name)) return name;
  const code = REMOTE_KEYS[name.toLowerCase()];
  if (code !== undefined) return code;
  const suggestion = closestMatch(name.toLowerCase(), REMOTE_KEY_NAMES);
  const hint = suggestion === null ? '' : `. Did you mean ${suggestion}?`;
  throw new UsageError(`Unknown key ${name}${hint}`, 'key');
}
