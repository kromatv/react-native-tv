const PRERELEASE = /^\d+\.\d+\.\d+-/;

/**
 * The npm dist-tag `version` publishes to. A prerelease goes to `next` once a
 * stable version is on npm; until then it takes `latest`, which an install
 * without a tag resolves.
 */
export function distTag(version: string, published: readonly string[]): 'latest' | 'next' {
  if (!PRERELEASE.test(version)) return 'latest';
  return published.some((other) => !PRERELEASE.test(other)) ? 'next' : 'latest';
}
