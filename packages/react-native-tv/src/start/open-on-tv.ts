import { lastTvTarget, rememberTvTarget } from './last-tv-target.ts';
import {
  providerTargets,
  tvStartProviders,
  type TvStartProvider,
  type TvStartRegistry,
} from './tv-start-registry.ts';

/** What the two keys read and write, replaced in tests. */
export interface TvOpenIo {
  readonly write: (text: string) => void;
  readonly registry: TvStartRegistry;
  readonly projectRoot: string;
  readonly lastTargetFile: string;
  /** Asks which of the labels to open, and gives its index, or null. */
  readonly ask: (labels: readonly string[]) => Promise<number | null>;
}

const NO_PLATFORM =
  'No TV platform package of this app offers a target here: each needs a version that registers with expo start';

/** Writes one line in the style of Expo's own messages. */
export function say(write: (text: string) => void, line: string): void {
  write(`› ${line}\n`);
}

async function open(provider: TvStartProvider, target: string | null, io: TvOpenIo): Promise<void> {
  try {
    await provider.open(target);
  } catch (error) {
    const reason = error instanceof Error ? error.message : String(error);
    say(io.write, `${provider.package} could not open the app: ${reason}`);
  }
}

/** `shift+t`: lists the targets of every platform, opens the one chosen and remembers it for `t`. */
export async function chooseTvTarget(io: TvOpenIo): Promise<void> {
  const providers = tvStartProviders(io.registry);
  if (providers.length === 0) {
    say(io.write, NO_PLATFORM);
    return;
  }
  const listed = await Promise.all(
    providers.map(async (provider) =>
      (await providerTargets(provider)).map((each) => ({ provider, ...each })),
    ),
  );
  const choices = listed.flat();
  if (choices.length === 0) {
    say(io.write, 'No TV or simulator to open on');
    return;
  }
  const index = await io.ask(choices.map((each) => `${each.provider.platform}  ${each.label}`));
  const chosen = index === null ? undefined : choices[index];
  if (chosen === undefined) return;
  const { platform } = chosen.provider;
  rememberTvTarget(io.lastTargetFile, io.projectRoot, { platform, target: chosen.target });
  await open(chosen.provider, chosen.target, io);
}

/**
 * `t`: opens where this project opened last. With nothing remembered, an app
 * with one platform opens on that platform's TV used last, and an app with
 * several asks.
 */
export async function openOnLastTv(io: TvOpenIo): Promise<void> {
  const providers = tvStartProviders(io.registry);
  const [only] = providers;
  if (only === undefined) {
    say(io.write, NO_PLATFORM);
    return;
  }
  const last = lastTvTarget(io.lastTargetFile, io.projectRoot);
  const remembered = providers.find((provider) => provider.platform === last?.platform);
  if (last !== null && remembered !== undefined) await open(remembered, last.target, io);
  else if (providers.length === 1) await open(only, null, io);
  else await chooseTvTarget(io);
}
