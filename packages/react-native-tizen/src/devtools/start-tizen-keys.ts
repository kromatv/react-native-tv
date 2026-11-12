import { TIZEN_PLUGIN } from '../config/tizen-settings.ts';
import { TIZEN_PLATFORM } from '../metro/with-tizen.ts';
import { colorsEnabled, painter } from '../paint.ts';
import type { DevServerPoll } from './dev-server-ready.ts';
import type { KeyInput } from './expo-key-input.ts';
import { jobsDir } from './jobs.ts';
import { openOnTizen, say, type OpenIo } from './open-on-tizen.ts';
import { listenForTizenKeys } from './tizen-keys.ts';
import {
  tvStartRegistry,
  type TvStartProvider,
  type TvStartRegistry,
} from './tv-start-registry.ts';
import { promptTizenTarget } from './tizen-prompt.ts';
import { lastOrOnlyTv, tizenTargets, type TvFinder } from './tizen-targets.ts';
import { tizenUsage } from './tizen-usage.ts';

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
 * platform can open on Tizen too. That package claims the keys, and this one
 * then adds none.
 */
export function addTizenKeys(terminal: ExpoTerminal): void {
  const { input, output, env } = terminal;
  const registry = terminal.registry ?? tvStartRegistry();
  const write = (text: string) => {
    output.write(text);
  };
  const io: OpenIo = { write, dir: terminal.dir, poll: terminal.poll };
  const finder: Partial<TvFinder> = {
    searching: () => say(write, 'Looking for Samsung TVs on the network...'),
    ...terminal.finder,
  };
  const paint = painter(colorsEnabled(output, env));
  const openLast = () => openOnTizen(() => lastOrOnlyTv(finder), env, io);
  const provider: TvStartProvider = {
    platform: TIZEN_PLATFORM,
    package: TIZEN_PLUGIN,
    targets: () => tizenTargets(finder),
    open: (target) =>
      target === null ? openLast() : openOnTizen(() => Promise.resolve(target), env, io),
  };
  registry.providers.set(TIZEN_PLATFORM, provider);
  listenForTizenKeys(input, {
    claimed: () => registry.claimed,
    open: openLast,
    choose: () =>
      openOnTizen(async () => promptTizenTarget(input, await tizenTargets(finder), write), env, io),
    usage: (verbose) => write(tizenUsage(paint, verbose)),
    busy: () => say(write, 'Already opening on Tizen: wait for it to finish'),
    defer: (work) => setImmediate(work),
  });
}

/**
 * Gives an interactive `expo start` the `t` and `shift+t` keys, once, as Expo
 * gives itself `a` and `shift+a`. Expo's terminal UI runs on a TTY outside CI only.
 */
export function startTizenKeys(terminal: ExpoTerminal = defaultTerminal()): void {
  const { input, output, env } = terminal;
  if (started || input.isTTY !== true || output.isTTY !== true || env.CI) return;
  started = true;
  addTizenKeys(terminal);
}
