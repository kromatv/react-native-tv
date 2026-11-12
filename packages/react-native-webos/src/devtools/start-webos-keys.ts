import { WEBOS_PLUGIN } from '../config/webos-settings.ts';
import { WEBOS_PLATFORM } from '../metro/with-webos.ts';
import { colorsEnabled, painter } from '../paint.ts';
import type { DevServerPoll } from './dev-server-ready.ts';
import type { KeyInput } from './expo-key-input.ts';
import { jobsDir } from './jobs.ts';
import { openOnWebOS, say, type OpenIo } from './open-on-webos.ts';
import { listenForWebOSKeys } from './webos-keys.ts';
import {
  tvStartRegistry,
  type TvStartProvider,
  type TvStartRegistry,
} from './tv-start-registry.ts';
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
  readonly registry?: TvStartRegistry;
  readonly poll?: DevServerPoll;
}

let started = false;

function defaultTerminal(): ExpoTerminal {
  return { input: process.stdin, output: process.stdout, env: process.env, dir: jobsDir() };
}

/**
 * Adds the `t` and `shift+t` keys to the `expo start` running in `terminal`,
 * and registers what they open, so a package that offers one key for every TV
 * platform can open on webOS too. That package claims the keys, and this one
 * then adds none.
 */
export function addWebOSKeys(terminal: ExpoTerminal): void {
  const { input, output, env } = terminal;
  const registry = terminal.registry ?? tvStartRegistry();
  const write = (text: string) => {
    output.write(text);
  };
  const io: OpenIo = { write, dir: terminal.dir, poll: terminal.poll };
  const finder: Partial<TvFinder> = {
    searching: () => say(write, 'Looking for LG TVs on the network...'),
    ...terminal.finder,
  };
  const paint = painter(colorsEnabled(output, env));
  const openLast = () => openOnWebOS(() => lastOrOnlyTv(finder), env, io);
  const provider: TvStartProvider = {
    platform: WEBOS_PLATFORM,
    package: WEBOS_PLUGIN,
    targets: () => webosTargets(finder),
    open: (target) =>
      target === null ? openLast() : openOnWebOS(() => Promise.resolve(target), env, io),
  };
  registry.providers.set(WEBOS_PLATFORM, provider);
  listenForWebOSKeys(input, {
    claimed: () => registry.claimed,
    open: openLast,
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
