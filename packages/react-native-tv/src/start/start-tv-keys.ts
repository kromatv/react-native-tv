import { colorsEnabled, painter } from '../paint.ts';
import type { KeyInput } from './expo-key-input.ts';
import { lastTargetFile } from './last-tv-target.ts';
import { chooseTvTarget, openOnLastTv, say, type TvOpenIo } from './open-on-tv.ts';
import { listenForTvKeys } from './tv-keys.ts';
import { promptTvTarget } from './tv-prompt.ts';
import { tvStartRegistry, type TvStartRegistry } from './tv-start-registry.ts';
import { tvUsage } from './tv-usage.ts';

/** The terminal `expo start` runs in, replaced in tests. */
export interface ExpoTerminal {
  readonly input: KeyInput & { readonly isTTY?: boolean };
  readonly output: { readonly isTTY?: boolean; write(text: string): unknown };
  readonly env: NodeJS.ProcessEnv;
  readonly registry: TvStartRegistry;
  readonly cwd: string;
}

let started = false;

function defaultTerminal(): ExpoTerminal {
  return {
    input: process.stdin,
    output: process.stdout,
    env: process.env,
    registry: tvStartRegistry(),
    cwd: process.cwd(),
  };
}

/**
 * Adds one `t` and one `shift+t` to the `expo start` running in `terminal`,
 * for every TV platform, and claims the registry so no platform package adds a
 * key of its own.
 */
export function addTvKeys(terminal: ExpoTerminal): void {
  const { input, output, env, registry } = terminal;
  const write = (text: string) => {
    output.write(text);
  };
  const io: TvOpenIo = {
    write,
    registry,
    projectRoot: env.EXPO_PUBLIC_PROJECT_ROOT ?? terminal.cwd,
    lastTargetFile: lastTargetFile(env),
    ask: (labels) => promptTvTarget(input, labels, write),
  };
  const paint = painter(colorsEnabled(output, env));
  registry.claimed = true;
  listenForTvKeys(input, {
    open: () => openOnLastTv(io),
    choose: () => chooseTvTarget(io),
    usage: (verbose) => write(tvUsage(paint, verbose)),
    busy: () => say(write, 'Already opening on a TV: wait for it to finish'),
    defer: (work) => setImmediate(work),
  });
}

/** Gives an interactive `expo start` the keys, once. Expo's terminal UI runs on a TTY outside CI only. */
export function startTvKeys(terminal: ExpoTerminal = defaultTerminal()): void {
  const { input, output, env } = terminal;
  if (started || input.isTTY !== true || output.isTTY !== true || env.CI) return;
  started = true;
  addTvKeys(terminal);
}
