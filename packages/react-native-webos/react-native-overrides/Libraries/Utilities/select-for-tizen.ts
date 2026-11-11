/** What `Platform.select` takes: a value per platform, `native`, and `default`. */
export interface PlatformSelectSpec<T> {
  readonly tizen?: T;
  readonly android?: T;
  readonly native?: T;
  readonly default?: T;
  readonly [platform: string]: T | undefined;
}

const PRECEDENCE = ['tizen', 'android', 'native', 'default'] as const;

/**
 * `Platform.select` on Tizen: the `tizen` entry, else `android`'s, since a
 * Tizen bundle runs React Native's Android sources, else `native`, else
 * `default`. An entry counts when its key is present, as in React Native.
 */
export function selectForTizen<T>(spec: PlatformSelectSpec<T>): T | undefined {
  const key = PRECEDENCE.find((candidate) => candidate in spec);
  return key === undefined ? undefined : spec[key];
}
