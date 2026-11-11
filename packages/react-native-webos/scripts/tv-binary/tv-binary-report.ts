import {
  ARM_ATTRIBUTES_SECTION,
  parseArmAttributes,
  type ArmAttributes,
} from './arm-attributes.ts';
import type { ElfFile } from './elf-file.ts';
import {
  checkFirmware,
  compatibilityFloor,
  isCompatible,
  type FirmwareResult,
} from './firmware-check.ts';
import type { Firmware } from './firmware-data.ts';
import { highestVersion, versionUses, type VersionUse } from './glibc-versions.ts';
import { runtimeLibraries, type RuntimeLibrary } from './runtime-lookups.ts';

const MACHINE_ARM = 40;
const TYPE_NAMES: Readonly<Record<number, string>> = {
  2: 'executable at a fixed address (ET_EXEC)',
  3: 'position-independent executable or shared object (ET_DYN)',
};
const FLOAT_SOFT = 0x200;
const FLOAT_HARD = 0x400;

/** Everything `check-tv-binary` reports about one executable. */
export interface TvBinaryReport {
  readonly architecture: string;
  readonly type: string;
  /** `e_flags`, in hexadecimal, and the float calling convention it declares. */
  readonly flags: string;
  readonly attributes: ArmAttributes | null;
  readonly interpreter: string | null;
  readonly needed: readonly string[];
  readonly versions: readonly VersionUse[];
  readonly highestGlibc: string | null;
  readonly highestLibstdcxx: string | null;
  readonly runtimeLibraries: readonly RuntimeLibrary[];
  readonly firmwares: readonly FirmwareResult[];
  /** The oldest webOS release from which every image has all the executable needs. */
  readonly floor: string | null;
}

function machineName(machine: number): string {
  return machine === MACHINE_ARM ? 'ARM' : `machine ${machine}`;
}

function describeFlags(elf: ElfFile): string {
  const hex = `0x${elf.flags.toString(16)}`;
  if (elf.machine !== MACHINE_ARM) return hex;
  const eabi = elf.flags >>> 24;
  let floats = 'no float ABI flag';
  if ((elf.flags & FLOAT_HARD) !== 0) floats = 'hard-float calling convention';
  else if ((elf.flags & FLOAT_SOFT) !== 0) floats = 'soft-float calling convention';
  return `${hex} (EABI${eabi}, ${floats})`;
}

/** Reads what a TV's dynamic linker will ask of `elf` and checks it against every firmware image. */
export function reportTvBinary(
  elf: ElfFile,
  firmwares: readonly Firmware[],
  opened: readonly string[],
): TvBinaryReport {
  const attributes = elf.sections.find((section) => section.type === ARM_ATTRIBUTES_SECTION);
  const versions = versionUses(elf.imports);
  const needs = { needed: elf.needed, imports: elf.imports, opened };
  const results = firmwares.map((firmware) => checkFirmware(firmware, needs));
  return {
    architecture: `${elf.is64 ? 'ELF64' : 'ELF32'} ${machineName(elf.machine)}`,
    type: TYPE_NAMES[elf.type] ?? `type ${elf.type}`,
    flags: describeFlags(elf),
    attributes: attributes === undefined ? null : parseArmAttributes(elf.sectionBytes(attributes)),
    interpreter: elf.interpreter,
    needed: elf.needed,
    versions,
    highestGlibc: highestVersion(versions, 'GLIBC_'),
    highestLibstdcxx: highestVersion(versions, 'GLIBCXX_'),
    runtimeLibraries: runtimeLibraries(elf, firmwares, opened),
    firmwares: results,
    floor: compatibilityFloor(results),
  };
}

function few(items: readonly string[], limit = 8): string {
  const shown = items.slice(0, limit).join(', ');
  return items.length > limit ? `${shown} and ${items.length - limit} more` : shown;
}

function attributeLine(attributes: ArmAttributes): string {
  return [
    attributes.cpuName === null
      ? attributes.cpuArch
      : `${attributes.cpuName} (${attributes.cpuArch})`,
    `profile ${attributes.profile}`,
    attributes.thumb,
    attributes.fpArch,
    attributes.simd,
    attributes.halfPrecision ? 'half-precision' : null,
    attributes.vfpArguments
      ? 'floats in VFP registers (hard)'
      : 'floats in core registers (softfp)',
  ]
    .filter((part) => part !== null)
    .join(', ');
}

function firmwareLine(result: FirmwareResult): string {
  const head = `  webOS ${result.release.padEnd(7)} ${result.otaId.slice(7, 11)}  `;
  if (isCompatible(result)) {
    const weak = result.missingWeakSymbols;
    if (weak.length === 0) return `${head}ok`;
    return `${head}ok (${weak.length} weak symbols absent: ${few(weak, 4)})`;
  }
  const problems = [
    result.missingLibraries.length > 0 ? `no ${few(result.missingLibraries)}` : null,
    result.missingVersions.length > 0 ? `no version ${few(result.missingVersions)}` : null,
    result.missingSymbols.length > 0
      ? `${result.missingSymbols.length} symbols missing: ${few(result.missingSymbols)}`
      : null,
  ].filter((part) => part !== null);
  return `${head}${problems.join('; ')}`;
}

function runtimeLibraryLines(library: RuntimeLibrary): string[] {
  const where =
    library.absentOn.length === 0 ? 'on every image' : `absent on ${few(library.absentOn, 14)}`;
  const required = library.required ? ' (required)' : '';
  const lines = [`  ${library.name}${required}: ${where}`];
  for (const [symbol, lacking] of library.lookups) {
    lines.push(`    ${symbol}: not on ${few(lacking, 14)}`);
  }
  return lines;
}

function floorLine(floor: string | null): string {
  return floor === null ? 'Floor: none, the newest image lacks something' : `Floor: webOS ${floor}`;
}

/** The report as the text `check-tv-binary` prints. */
export function formatTvBinaryReport(report: TvBinaryReport): string {
  const versionLines = report.versions.map(
    (use) =>
      `  ${use.library} ${use.version}: ${use.symbols.length} symbols (${few(use.symbols, 4)})`,
  );
  return [
    `${report.architecture}, ${report.type}, flags ${report.flags}`,
    `Build attributes: ${report.attributes === null ? 'none' : attributeLine(report.attributes)}`,
    `Interpreter: ${report.interpreter ?? 'none'}`,
    `Needs: ${report.needed.join(' ')}`,
    'Symbol versions:',
    ...versionLines,
    `  highest glibc version: ${report.highestGlibc ?? 'none'}`,
    `  highest libstdc++ version: ${report.highestLibstdcxx ?? 'none imported (linked statically)'}`,
    'Firmware images:',
    ...report.firmwares.map(firmwareLine),
    'Library names among the strings of the executable (opened at run time, if at all); for a required one, the strings that are its symbols on some image but not on all:',
    ...report.runtimeLibraries.flatMap(runtimeLibraryLines),
    floorLine(report.floor),
  ].join('\n');
}
