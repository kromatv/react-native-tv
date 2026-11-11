import { fileURLToPath } from 'node:url';

const fromPackage = (path: string) => fileURLToPath(new URL(path, import.meta.url));

/** This package's root directory, wherever it is installed. */
export const PACKAGE_ROOT = fromPackage('..');

/** The CMake toolchain file every native build for webOS goes through. */
export const TOOLCHAIN_FILE = fromPackage('../runtime/cmake/toolchain/webos.cmake');

/** Sources of the native runtime and its helpers. */
export const RUNTIME_DIR = fromPackage('../runtime');

/** The pinned C++ dependencies, read by CMake and by `rn-webos`. */
export const DEPENDENCIES_FILE = fromPackage('../runtime/dependencies.json');

/** The licences of what a package carries, copied into every package's `res/`. */
export const NOTICES_FILE = fromPackage('../THIRD_PARTY_NOTICES.md');

/** The .NET shell project a webOS package is generated from. */
export const SHELL_DIR = fromPackage('../shell');
