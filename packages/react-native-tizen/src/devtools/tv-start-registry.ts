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

/**
 * Where the TV platform packages of an app meet inside `expo start`: each
 * registers a provider under its platform, and a package that offers one key
 * for all of them sets `claimed`, after which no platform adds a key of its own.
 */
export interface TvStartRegistry {
  claimed: boolean;
  readonly providers: Map<string, unknown>;
}

const registrySchema = z.custom<TvStartRegistry>(
  (value) =>
    z.looseObject({ claimed: z.boolean(), providers: z.instanceof(Map) }).safeParse(value).success,
);

/** The registry of this process, made on first use. A value left there in another shape is replaced. */
export function tvStartRegistry(host: object = globalThis): TvStartRegistry {
  const existing = registrySchema.safeParse(Reflect.get(host, TV_START_REGISTRY));
  if (existing.success) return existing.data;
  const registry: TvStartRegistry = { claimed: false, providers: new Map() };
  Reflect.set(host, TV_START_REGISTRY, registry);
  return registry;
}
