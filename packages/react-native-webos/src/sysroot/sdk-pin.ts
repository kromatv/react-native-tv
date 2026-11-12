/**
 * The SDK the TV runtime is compiled against: openlgtv's buildroot-nc4, glibc
 * 2.12.2 with GCC 16.2.0's libstdc++ (docs/reference/webos-native-toolchain.md).
 * The archive is built for a Linux host, which does not matter: clang reads its
 * target files only.
 */
export const SDK_PIN = {
  version: '2026.08-webos',
  url: 'https://github.com/openlgtv/buildroot-nc4/releases/download/2026.08-webos/arm-webos-linux-gnueabi_sdk-buildroot-aarch64.tar.gz',
  sha256: '3a9330f3789f8008ece7c8a48f58c3a71d0307a9a1f81896db938b785524c3ce',
  /** The one directory at the top of the archive. */
  root: 'arm-webos-linux-gnueabi_sdk-buildroot',
} as const;

/** What a build reads of the SDK, relative to its root: the sysroot, the C++ headers, the static runtime and GCC's own files. */
export const SDK_DIRECTORIES = [
  'arm-webos-linux-gnueabi/sysroot',
  'arm-webos-linux-gnueabi/include',
  'arm-webos-linux-gnueabi/lib',
  'lib/gcc/arm-webos-linux-gnueabi/16.2.0',
] as const;

/** Parts of the sysroot no build reads; `terminfo` also holds names that differ by case only, which macOS cannot keep apart. */
export const SDK_EXCLUDED = ['doc', 'i18n', 'locale', 'man', 'terminfo', 'zoneinfo'].map(
  (name) => `arm-webos-linux-gnueabi/sysroot/usr/share/${name}`,
);
