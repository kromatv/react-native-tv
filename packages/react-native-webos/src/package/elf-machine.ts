const ELF_MAGIC = [0x7f, 0x45, 0x4c, 0x46] as const;
const ELF_DATA_BIG_ENDIAN = 2;
const HEADER_BYTES = 20;

/** `e_machine` of 32-bit ARM, the one CPU a webOS TV runs apps on. */
export const ELF_MACHINE_ARM = 40;

/** The `e_machine` of an ELF file's first bytes, or null when they are not an ELF header. */
export function elfMachine(file: Uint8Array): number | null {
  if (file.byteLength < HEADER_BYTES || ELF_MAGIC.some((byte, index) => file[index] !== byte)) {
    return null;
  }
  const view = new DataView(file.buffer, file.byteOffset, file.byteLength);
  return view.getUint16(18, file[5] !== ELF_DATA_BIG_ENDIAN);
}
