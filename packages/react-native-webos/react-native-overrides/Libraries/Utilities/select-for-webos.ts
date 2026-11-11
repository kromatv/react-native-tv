/** What `Platform.select` takes: a value per platform, `native`, and `default`. */
export interface PlatformSelectSpec<T> {
  readonly webos?: T;
  readonly android?: T;
  readonly native?: T;
  readonly default?: T;
  readonly [platform: string]: T | undefined;
}

const PRECEDENCE = ['webos', 'android', 'native', 'default'] as const;

/**
 * `Platform.select` on webOS: the `webos` entry, else `android`'s, since a
 * webOS bundle runs React Native's Android sources, else `native`, else
 * `default`. An entry counts when its key is present, as in React Native.
 */
export function selectForWebOS<T>(spec: PlatformSelectSpec<T>): T | undefined {
  const key = PRECEDENCE.find((candidate) => candidate in spec);
  return key === undefined ? undefined : spec[key];
}
