const SECTION_ARM_ATTRIBUTES = 0x70000003;
const FILE_SCOPE = 1;

// Addenda to the ABI for the Arm Architecture, "Build attributes": tags 4, 5,
// 65 and 67 hold a string, 32 a number then a string; of the others, tags
// above 32 hold a string when odd.
const STRING_TAGS = new Set([4, 5, 65, 67]);
const COMPATIBILITY_TAG = 32;

const TAG = {
  cpuName: 5,
  cpuArch: 6,
  cpuProfile: 7,
  armUse: 8,
  thumbUse: 9,
  fpArch: 10,
  simdArch: 12,
  vfpArgs: 28,
  halfPrecision: 36,
} as const;

const CPU_ARCH = ['pre-v4', 'v4', 'v4T', 'v5T', 'v5TE', 'v5TEJ', 'v6', 'v6KZ', 'v6T2', 'v6K', 'v7'];
const FP_ARCH = ['none', 'VFPv1', 'VFPv2', 'VFPv3', 'VFPv3-D16', 'VFPv4', 'VFPv4-D16', 'ARMv8'];
const SIMD_ARCH = ['none', 'NEONv1', 'NEONv1 with FMA', 'ARMv8'];
const THUMB_USE = ['none', 'Thumb-1', 'Thumb-2'];

/** The ABI a 32-bit ARM file says it was compiled for, from its `.ARM.attributes` section. */
export interface ArmAttributes {
  readonly cpuName: string | null;
  readonly cpuArch: string;
  readonly profile: string;
  readonly armInstructions: boolean;
  readonly thumb: string;
  readonly fpArch: string;
  readonly simd: string;
  readonly halfPrecision: boolean;
  /** True when floats are passed in VFP registers (`gnueabihf`); false for the `softfp` and `soft` calling convention. */
  readonly vfpArguments: boolean;
}

/** `sectionType` of the section {@link parseArmAttributes} reads. */
export const ARM_ATTRIBUTES_SECTION = SECTION_ARM_ATTRIBUTES;

class Cursor {
  constructor(
    private readonly bytes: Uint8Array,
    public at: number,
  ) {}

  uleb(): number {
    let value = 0;
    let shift = 0;
    for (;;) {
      const byte = this.bytes[this.at];
      if (byte === undefined) return value;
      this.at += 1;
      value += (byte & 0x7f) * 2 ** shift;
      shift += 7;
      if ((byte & 0x80) === 0) return value;
    }
  }

  string(): string {
    const start = this.at;
    while (this.at < this.bytes.length && this.bytes[this.at] !== 0) this.at += 1;
    const text = new TextDecoder().decode(this.bytes.subarray(start, this.at));
    this.at += 1;
    return text;
  }
}

function readTags(bytes: Uint8Array, start: number, end: number) {
  const numbers = new Map<number, number>();
  const strings = new Map<number, string>();
  const cursor = new Cursor(bytes, start);
  while (cursor.at < end) {
    const tag = cursor.uleb();
    if (tag === COMPATIBILITY_TAG) {
      cursor.uleb();
      cursor.string();
    } else if (STRING_TAGS.has(tag) || (tag > 32 && tag % 2 === 1)) {
      strings.set(tag, cursor.string());
    } else {
      numbers.set(tag, cursor.uleb());
    }
  }
  return { numbers, strings };
}

/** The file-scope `aeabi` attributes of a section's bytes, or null when it holds none. */
export function parseArmAttributes(section: Uint8Array): ArmAttributes | null {
  if (section[0] !== 0x41) return null;
  const view = new DataView(section.buffer, section.byteOffset, section.byteLength);
  let at = 1;
  while (at + 4 <= section.length) {
    const end = at + view.getUint32(at, true);
    const vendor = new Cursor(section, at + 4);
    const isAeabi = vendor.string() === 'aeabi';
    let sub = vendor.at;
    while (isAeabi && sub + 5 <= end) {
      const subEnd = sub + view.getUint32(sub + 1, true);
      if (section[sub] === FILE_SCOPE) return describe(readTags(section, sub + 5, subEnd));
      sub = subEnd;
    }
    at = end;
  }
  return null;
}

function describe({ numbers, strings }: ReturnType<typeof readTags>): ArmAttributes {
  const number = (tag: number) => numbers.get(tag) ?? 0;
  const named = (names: readonly string[], value: number) => names[value] ?? `value ${value}`;
  const profile = number(TAG.cpuProfile);
  return {
    cpuName: strings.get(TAG.cpuName) ?? null,
    cpuArch: named(CPU_ARCH, number(TAG.cpuArch)),
    profile: profile === 0 ? 'none' : String.fromCharCode(profile),
    armInstructions: number(TAG.armUse) === 1,
    thumb: named(THUMB_USE, number(TAG.thumbUse)),
    fpArch: named(FP_ARCH, number(TAG.fpArch)),
    simd: named(SIMD_ARCH, number(TAG.simdArch)),
    halfPrecision: number(TAG.halfPrecision) === 1,
    vfpArguments: number(TAG.vfpArgs) === 1,
  };
}
