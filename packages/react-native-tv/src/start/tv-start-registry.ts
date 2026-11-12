import { z } from 'zod';

/** The key of the registry on `globalThis`, shared by every package in the `expo start` process. */
export const TV_START_REGISTRY = Symbol.for('reactNativeTvPlatform.start');

/** Something `expo start` can open the app on: a TV or a simulator, as its platform names it. */
export interface TvStartTarget {
  readonly target: string;
  readonly label: string;
}

/** What a TV platform package offers `expo start`, registered when its dev tools plugin loads. */
export interface TvStartProvider {
  readonly platform: string;
  readonly package: string;
  targets(): Promise<readonly TvStartTarget[]>;
  /** Opens the app on `target`, or on the TV used last for null. It prints its own progress and failures. */
  open(target: string | null): Promise<void>;
}

/** The registry itself: who offers targets, and whether one package handles the keys for all. */
export interface TvStartRegistry {
  claimed: boolean;
  readonly providers: Map<string, unknown>;
}

const registrySchema = z.custom<TvStartRegistry>(
  (value) =>
    z.looseObject({ claimed: z.boolean(), providers: z.instanceof(Map) }).safeParse(value).success,
);
const providerSchema = z.looseObject({
  platform: z.string().min(1).max(32),
  package: z.string().min(1).max(214),
  targets: z.custom<TvStartProvider['targets']>((value) => typeof value === 'function'),
  open: z.custom<TvStartProvider['open']>((value) => typeof value === 'function'),
});
const targetsSchema = z
  .array(z.object({ target: z.string().min(1).max(256), label: z.string().max(256) }))
  .max(64);

/** The registry of this process, made on first use. A value another package left there in another shape is replaced. */
export function tvStartRegistry(host: object = globalThis): TvStartRegistry {
  const existing = registrySchema.safeParse(Reflect.get(host, TV_START_REGISTRY));
  if (existing.success) return existing.data;
  const registry: TvStartRegistry = { claimed: false, providers: new Map() };
  Reflect.set(host, TV_START_REGISTRY, registry);
  return registry;
}

/** The providers registered in the shape this package knows, in the order of their platforms. */
export function tvStartProviders(registry: TvStartRegistry): TvStartProvider[] {
  return [...registry.providers.values()]
    .flatMap((value) => {
      const provider = providerSchema.safeParse(value);
      return provider.success ? [provider.data] : [];
    })
    .sort((first, second) => first.platform.localeCompare(second.platform));
}

/** A provider's targets, checked: a provider that fails or answers in another shape has none. */
export async function providerTargets(provider: TvStartProvider): Promise<TvStartTarget[]> {
  try {
    const targets = targetsSchema.safeParse(await provider.targets());
    return targets.success ? targets.data : [];
  } catch {
    return [];
  }
}
