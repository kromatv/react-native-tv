import { z } from 'zod';

/** A CPU a Tizen TV userland runs on. `x86_64` is the emulator. */
export const tizenArchSchema = z.enum(['armv7l', 'aarch64', 'x86_64']);
export type TizenArch = z.infer<typeof tizenArchSchema>;

/** A Tizen platform version with a native rootstrap, such as `9.0`. */
export const platformVersionSchema = z
  .string()
  .regex(/^\d+\.\d+$/, 'expected a version such as 9.0');

/** The CPU and platform a build is for. The platform is the OLDEST a package supports. */
export interface TizenTarget {
  readonly arch: TizenArch;
  readonly platformVersion: string;
}

function versionKey(version: string): number {
  const [major, minor] = `${version}.0`.split('.');
  return Number(major) * 1000 + Number(minor);
}

/** Orders two `major.minor` Tizen versions. */
export function compareTizenVersions(left: string, right: string): number {
  return versionKey(left) - versionKey(right);
}

/** A Tizen Studio rootstrap: where it is installed and the package that installs it. */
export interface Rootstrap {
  readonly profile: 'tizen' | 'iot-headed';
  readonly platformVersion: string;
  readonly name: string;
  readonly sdkPackage: string;
}

const ROOTSTRAP_KIND: Record<TizenArch, string> = {
  armv7l: 'device',
  aarch64: 'device64',
  x86_64: 'emulator64',
};

// Tizen Studio publishes no TV rootstrap. From 8.0 it has one `tizen` profile;
// before, IoT Headed is the profile with armv7 and aarch64 rootstraps (mobile
// has armv7 only). The only x86_64 TV emulator is 10.0's, and the 8.0 and 9.0
// x86_64 rootstraps carry IA-64's asm headers.
const UNIFIED_PROFILE_SINCE = '8.0';
const FIRST_EMULATOR64 = '10.0';

/** The rootstrap a target is compiled and linked against. */
export function rootstrapFor({ arch, platformVersion }: TizenTarget): Rootstrap {
  const version =
    arch === 'x86_64' && compareTizenVersions(platformVersion, FIRST_EMULATOR64) < 0
      ? FIRST_EMULATOR64
      : platformVersion;
  const kind = ROOTSTRAP_KIND[arch];
  if (compareTizenVersions(version, UNIFIED_PROFILE_SINCE) >= 0) {
    return {
      profile: 'tizen',
      platformVersion: version,
      name: `tizen-${version}-${kind}.core`,
      sdkPackage: `TIZEN-${version}-NativeAppDevelopment-CLI`,
    };
  }
  return {
    profile: 'iot-headed',
    platformVersion: version,
    name: `iot-headed-${version}-${kind}.core`,
    sdkPackage: `IOT-Headed-${version}-NativeAppDevelopment-CLI`,
  };
}

const DEVICE_ARCH: Record<string, TizenArch> = {
  armv7: 'armv7l',
  armv7l: 'armv7l',
  arm: 'armv7l',
  aarch64: 'aarch64',
  arm64: 'aarch64',
  x86_64: 'x86_64',
};

/** Maps the `cpu_arch` a TV reports through sdb's `capability:` service to a build arch. */
export function archFromDevice(cpuArch: string): TizenArch | null {
  return DEVICE_ARCH[cpuArch.trim()] ?? null;
}

export const DEFAULT_TARGET: TizenTarget = { arch: 'armv7l', platformVersion: '6.0' };
