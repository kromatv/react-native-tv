import type { TvStartProvider, TvStartTarget } from './tv-start-registry.ts';

/** A provider that records what it was asked to open. */
export interface FakeProvider extends TvStartProvider {
  readonly opened: (string | null)[];
}

export function fakeProvider(platform: string, targets: readonly TvStartTarget[]): FakeProvider {
  const opened: (string | null)[] = [];
  return {
    platform,
    package: `@tv/${platform}`,
    targets: () => Promise.resolve(targets),
    open: (target) => {
      opened.push(target);
      return Promise.resolve();
    },
    opened,
  };
}
