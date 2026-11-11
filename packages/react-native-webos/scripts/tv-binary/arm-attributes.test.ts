import { describe, expect, it } from 'vitest';
import { parseArmAttributes } from './arm-attributes.ts';

function section(vendor: string, attributes: readonly number[]): Uint8Array {
  const name = [...new TextEncoder().encode(vendor), 0];
  const fileScope = [1, 5 + attributes.length, 0, 0, 0, ...attributes];
  const length = 4 + name.length + fileScope.length;
  return new Uint8Array([0x41, length, 0, 0, 0, ...name, ...fileScope]);
}

const CPU_NAME = [5, 0x37, 0x2d, 0x41, 0];
const SOFTFP_NEON = [...CPU_NAME, 6, 10, 7, 0x41, 8, 1, 9, 2, 10, 3, 12, 1, 36, 1];

describe('parseArmAttributes', () => {
  it('reads the ABI a webOS TV build declares', () => {
    expect(parseArmAttributes(section('aeabi', SOFTFP_NEON))).toEqual({
      cpuName: '7-A',
      cpuArch: 'v7',
      profile: 'A',
      armInstructions: true,
      thumb: 'Thumb-2',
      fpArch: 'VFPv3',
      simd: 'NEONv1',
      halfPrecision: true,
      vfpArguments: false,
    });
  });

  it('tells a hard-float build by Tag_ABI_VFP_args', () => {
    expect(parseArmAttributes(section('aeabi', [...SOFTFP_NEON, 28, 1]))?.vfpArguments).toBe(true);
  });

  it('skips the string of Tag_compatibility and of odd tags above 32', () => {
    const attributes = [32, 1, 0x67, 0, 67, 0x32, 0x2e, 0x30, 0x39, 0, 10, 4];
    expect(parseArmAttributes(section('aeabi', attributes))?.fpArch).toBe('VFPv3-D16');
  });

  it('reads a tag number of several bytes', () => {
    expect(parseArmAttributes(section('aeabi', [0x80, 0x02, 7, 10, 5]))?.fpArch).toBe('VFPv4');
  });

  it('ignores another vendor and a section that is not build attributes', () => {
    expect(parseArmAttributes(section('gnu', SOFTFP_NEON))).toBeNull();
    expect(parseArmAttributes(new Uint8Array([0x42, 0, 0, 0, 0]))).toBeNull();
  });
});
