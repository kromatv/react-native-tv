import { existsSync, readdirSync } from 'node:fs';
import { homedir } from 'node:os';
import { join } from 'node:path';
import { FixableError } from '../errors.ts';
import { rootstrapName, type TizenArch, type TizenTarget } from './target.ts';

/** The pieces of a Tizen Studio installation this tool drives. */
export interface TizenSdk {
  readonly root: string;
  readonly sdb: string;
  readonly tizenCli: string;
  readonly packageManager: string;
}

const GCC_PREFIX: Record<TizenArch, string> = {
  armv7l: 'arm-linux-gnueabi-gcc-',
  aarch64: 'aarch64-linux-gnu-gcc-',
  x86_64: 'x86_64-linux-gnu-gcc-',
};

function packageManagerInstall(sdk: TizenSdk, pkg: string): string {
  return `${sdk.packageManager} install ${pkg} --accept-license`;
}

/** Finds Tizen Studio from `TIZEN_SDK_DIR`, then `~/tizen-studio`. */
export function locateTizenSdk(env: NodeJS.ProcessEnv = process.env): TizenSdk {
  const root = env.TIZEN_SDK_DIR ?? join(homedir(), 'tizen-studio');
  const sdb = join(root, 'tools', 'sdb');
  if (!existsSync(sdb)) {
    throw new FixableError(
      `Tizen Studio not found at ${root}`,
      'Install Tizen Studio (https://developer.tizen.org/development/tizen-studio/download), or set TIZEN_SDK_DIR to where it is.',
    );
  }
  return {
    root,
    sdb,
    tizenCli: join(root, 'tools', 'ide', 'bin', 'tizen'),
    packageManager: join(root, 'package-manager', 'package-manager-cli.bin'),
  };
}

/** The installed rootstrap for a target, or a {@link FixableError} naming the package to install. */
export function locateRootstrap(sdk: TizenSdk, target: TizenTarget): string {
  const path = join(
    sdk.root,
    'platforms',
    `tizen-${target.platformVersion}`,
    'tizen',
    'rootstraps',
    rootstrapName(target),
  );
  if (existsSync(path)) return path;
  const nativePackage = `TIZEN-${target.platformVersion}-NativeAppDevelopment-CLI`;
  throw new FixableError(
    `No Tizen ${target.platformVersion} rootstrap for ${target.arch}`,
    `Install it: ${packageManagerInstall(sdk, nativePackage)}`,
  );
}

/** The newest GCC Tizen Studio installed for an arch, whose runtime every native build links. */
export function locateGccToolchain(sdk: TizenSdk, arch: TizenArch): string {
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
    `No GCC toolchain for ${arch} in Tizen Studio`,
    `Install it: ${packageManagerInstall(sdk, 'NativeToolchain-Gcc-14.2')}`,
  );
}
