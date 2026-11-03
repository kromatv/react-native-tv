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

const ROOTSTRAP_KIND: Record<TizenArch, string> = {
  armv7l: 'device',
  aarch64: 'device64',
  x86_64: 'emulator64',
};

/** The rootstrap directory name Tizen Studio installs for a target. */
export function rootstrapName({ arch, platformVersion }: TizenTarget): string {
  return `tizen-${platformVersion}-${ROOTSTRAP_KIND[arch]}.core`;
}

const DEVICE_ARCH: Record<string, TizenArch> = {
  armv7: 'armv7l',
  armv7l: 'armv7l',
  arm: 'armv7l',
  aarch64: 'aarch64',
  arm64: 'aarch64',
  x86_64: 'x86_64',
};

/** Maps the `cpu_arch` a device reports through `sdb capability` to a build arch. */
export function archFromDevice(cpuArch: string): TizenArch | null {
  return DEVICE_ARCH[cpuArch.trim()] ?? null;
}

export const DEFAULT_TARGET: TizenTarget = { arch: 'armv7l', platformVersion: '9.0' };
