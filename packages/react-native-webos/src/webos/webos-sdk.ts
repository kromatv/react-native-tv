import { existsSync, readdirSync } from 'node:fs';
import { homedir } from 'node:os';
import { join } from 'node:path';
import { FixableError } from '../errors.ts';
import { rootstrapFor, type WebOSArch, type WebOSTarget } from './target.ts';

/** The pieces of a webOS Studio installation a build reads: its rootstrap and GCC runtime. */
export interface WebOSSdk {
  readonly root: string;
  readonly packageManager: string;
}

const GCC_PREFIX: Record<WebOSArch, string> = {
  armv7l: 'arm-linux-gnueabi-gcc-',
  aarch64: 'aarch64-linux-gnu-gcc-',
  x86_64: 'x86_64-linux-gnu-gcc-',
};

function packageManagerInstall(sdk: WebOSSdk, pkg: string): string {
  return `${sdk.packageManager} install ${pkg} --accept-license`;
}

/** Finds webOS Studio from `WEBOS_SDK_DIR`, then `~/webos-studio`. */
export function locateWebOSSdk(env: NodeJS.ProcessEnv = process.env): WebOSSdk {
  const root = env.WEBOS_SDK_DIR ?? join(homedir(), 'webos-studio');
  if (!existsSync(join(root, 'sdk.version'))) {
    throw new FixableError(
      `webOS Studio not found at ${root}`,
      'Install webOS Studio (https://developer.webos.org/development/webos-studio/download), or set WEBOS_SDK_DIR to where it is.',
    );
  }
  return {
    root,
    packageManager: join(root, 'package-manager', 'package-manager-cli.bin'),
  };
}

/** The installed rootstrap for a target, or a {@link FixableError} naming the package to install. */
export function locateRootstrap(sdk: WebOSSdk, target: WebOSTarget): string {
  const rootstrap = rootstrapFor(target);
  const path = join(
    sdk.root,
    'platforms',
    `webos-${rootstrap.platformVersion}`,
    rootstrap.profile,
    'rootstraps',
    rootstrap.name,
  );
  if (existsSync(path)) return path;
  throw new FixableError(
    `No webOS ${rootstrap.platformVersion} rootstrap for ${target.arch}`,
    `Install it: ${packageManagerInstall(sdk, rootstrap.sdkPackage)}`,
  );
}

/** The newest GCC webOS Studio installed for an arch, whose runtime every native build links. */
export function locateGccToolchain(sdk: WebOSSdk, arch: WebOSArch): string {
  const tools = join(sdk.root, 'tools');
  const prefix = GCC_PREFIX[arch];
  const installed = existsSync(tools)
    ? readdirSync(tools)
        .filter((name) => name.startsWith(prefix))
        .sort((left, right) => left.localeCompare(right, 'en', { numeric: true }))
    : [];
  const newest = installed.at(-1);
  if (newest !== undefined) return join(tools, newest);
  throw new FixableError(
    `No GCC toolchain for ${arch} in webOS Studio`,
    `Install it: ${packageManagerInstall(sdk, 'NativeToolchain-Gcc-14.2')}`,
  );
}
