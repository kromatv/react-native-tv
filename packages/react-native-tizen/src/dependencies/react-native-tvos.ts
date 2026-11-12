/**
 * The `react-native` an app installs for this package: react-native-tvos, at
 * the release the runtime is built and verified against. An exact version,
 * because Bun 1.4 fails on Expo's `0.88-stable` tag in an alias.
 */
export const REACT_NATIVE_TVOS_SPEC = 'npm:react-native-tvos@0.88.0-0';

/** True when a `react-native` dependency spec installs react-native-tvos. */
export function isReactNativeTvosSpec(spec: string): boolean {
  return spec.startsWith('npm:react-native-tvos@');
}
