import { z } from 'zod';

/** The packages a canary carries, renderers first: the order they reach npm in. */
export const CANARY_PACKAGES = [
  'react-native-tizen',
  'react-native-webos',
  'react-native-tv',
] as const;

const SHA = /^[0-9a-f]{7,40}$/;
const PackageJson = z.looseObject({
  name: z.string(),
  version: z.string(),
  dependencies: z.record(z.string(), z.string()).optional(),
});

/**
 * The canary version of build `build` of commit `sha`. It sits on 0.0.0, below
 * every release, so no `^` or `~` range on a release ever resolves to it: only
 * the `canary` dist-tag does.
 */
export function canaryVersion(build: number, sha: string): string {
  if (!Number.isSafeInteger(build) || build < 1) throw new Error(`${build} is not a build number`);
  if (!SHA.test(sha)) throw new Error(`${sha} is not a commit`);
  return `0.0.0-canary.${build}.g${sha.slice(0, 7)}`;
}

/**
 * `manifest` as the canary `version` publishes it: that version, and the same
 * version for every dependency on another canary package, so one canary of
 * react-native-tv installs the renderers built from its own commit.
 */
export function stampCanary(manifest: string, version: string): string {
  const fields: Record<string, unknown> = JSON.parse(manifest);
  const { dependencies } = PackageJson.parse(fields);
  const names = new Set(CANARY_PACKAGES.map((name) => `@kromatv/${name}`));
  const stamped: Record<string, unknown> = { ...fields, version };
  if (dependencies) {
    stamped.dependencies = Object.fromEntries(
      Object.entries(dependencies).map(([name, range]) => [
        name,
        names.has(name) ? version : range,
      ]),
    );
  }
  return `${JSON.stringify(stamped, null, 2)}\n`;
}
