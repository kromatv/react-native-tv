import { colorsEnabled, painter } from '../paint.ts';
import type { KeyInput } from './expo-key-input.ts';
import { jobsDir } from './jobs.ts';
import { openOnWebOS, say, type OpenIo } from './open-on-webos.ts';
import { listenForWebOSKeys } from './webos-keys.ts';
import { promptWebOSTarget } from './webos-prompt.ts';
import { lastOrOnlyTv, webosTargets, type TvFinder } from './webos-targets.ts';
import { webosUsage } from './webos-usage.ts';

/** The terminal `expo start` runs in, replaced in tests. */
export interface ExpoTerminal {
  readonly input: KeyInput & { readonly isTTY?: boolean };
  readonly output: { readonly isTTY?: boolean; write(text: string): unknown };
  readonly env: NodeJS.ProcessEnv;
  readonly dir: string;
  readonly finder?: Partial<TvFinder>;
}

let started = false;

function defaultTerminal(): ExpoTerminal {
  return { input: process.stdin, output: process.stdout, env: process.env, dir: jobsDir() };
}

/** Adds the `t` and `shift+t` keys to the `expo start` running in `terminal`. */
export function addWebOSKeys(terminal: ExpoTerminal): void {
  const { input, output, env } = terminal;
  const write = (text: string) => {
    output.write(text);
  };
  const io: OpenIo = { write, dir: terminal.dir };
  const finder: Partial<TvFinder> = {
    searching: () => say(write, 'Looking for Samsung TVs on the network...'),
    ...terminal.finder,
  };
  const paint = painter(colorsEnabled(output, env));
  listenForWebOSKeys(input, {
    open: () => openOnWebOS(() => lastOrOnlyTv(finder), env, io),
    choose: () =>
      openOnWebOS(async () => promptWebOSTarget(input, await webosTargets(finder), write), env, io),
    usage: (verbose) => write(webosUsage(paint, verbose)),
    busy: () => say(write, 'Already opening on webOS: wait for it to finish'),
    defer: (work) => setImmediate(work),
  });
}

/**
 * Gives an interactive `expo start` the `t` and `shift+t` keys, once, as Expo
 * gives itself `a` and `shift+a`. Expo's terminal UI runs on a TTY outside CI only.
 */
export function startWebOSKeys(terminal: ExpoTerminal = defaultTerminal()): void {
  const { input, output, env } = terminal;
  if (started || input.isTTY !== true || output.isTTY !== true || env.CI) return;
  started = true;
  addWebOSKeys(terminal);
}
