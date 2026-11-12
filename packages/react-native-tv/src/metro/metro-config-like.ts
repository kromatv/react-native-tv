/** The part of a Metro config {@link TvPlatformWrapper} is given: each platform reads and extends what it needs. */
export interface MetroConfigLike {
  readonly projectRoot?: string;
  readonly [key: string]: unknown;
}

/** Teaches a Metro config one TV platform, and returns it. */
export type TvPlatformWrapper = <Config extends MetroConfigLike>(config: Config) => Config;
