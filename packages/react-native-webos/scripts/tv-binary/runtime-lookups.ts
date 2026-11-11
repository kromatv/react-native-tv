import type { ElfFile } from './elf-file.ts';
import type { Firmware } from './firmware-data.ts';

const LIBRARY_NAME = /^lib[A-Za-z0-9_+.-]+\.so(\.\d+)*$/;
const SYMBOL_NAME = /^[A-Za-z_][A-Za-z0-9_]{2,}$/;
const STRING_SECTIONS = new Set(['.rodata', '.data.rel.ro', '.data']);
const MAX_STRING = 256;

/** Every printable, NUL-terminated string in the sections a string literal lands in. */
export function literalStrings(elf: ElfFile): Set<string> {
  const strings = new Set<string>();
  const decoder = new TextDecoder();
  for (const section of elf.sections) {
    if (!STRING_SECTIONS.has(section.name)) continue;
    const bytes = elf.sectionBytes(section);
    let start = 0;
    for (let at = 0; at < bytes.length; at += 1) {
      const byte = bytes[at] ?? 0;
      if (byte >= 0x20 && byte < 0x7f) continue;
      if (byte === 0 && at - start >= 3 && at - start <= MAX_STRING) {
        strings.add(decoder.decode(bytes.subarray(start, at)));
      }
      start = at + 1;
    }
  }
  return strings;
}

/** A library the executable may open with `dlopen`, and where it exists. */
export interface RuntimeLibrary {
  readonly name: string;
  /** True for a library the caller says the code cannot run without. */
  readonly required: boolean;
  /** Releases whose image has no such library. */
  readonly absentOn: readonly string[];
  /**
   * For a required library: strings of the executable that are symbols of it
   * on some image, with the releases on which a `dlsym` for one finds nothing.
   */
  readonly lookups: ReadonlyMap<string, readonly string[]>;
}

/** What `dlsym` on a handle of `name` can find: the library's symbols, then those of what it depends on. */
function reachableNames(firmware: Firmware, name: string): Set<string> | null {
  if (firmware.library(name) === null) return null;
  const names = new Set<string>();
  const seen = new Set<string>();
  const queue = [name];
  for (let next = queue.shift(); next !== undefined; next = queue.shift()) {
    if (seen.has(next)) continue;
    seen.add(next);
    const library = firmware.library(next);
    if (library === null) continue;
    for (const symbol of library.names) names.add(symbol);
    queue.push(...library.needed);
  }
  return names;
}

function lookupsIn(
  name: string,
  candidates: readonly string[],
  firmwares: readonly Firmware[],
): Map<string, string[]> {
  const own = new Set(firmwares.flatMap((firmware) => [...(firmware.library(name)?.names ?? [])]));
  const reachable = firmwares.map((firmware) => ({
    release: firmware.release,
    names: reachableNames(firmware, name),
  }));
  const lookups = new Map<string, string[]>();
  for (const candidate of candidates) {
    if (!own.has(candidate)) continue;
    const lacking = reachable
      .filter(({ names }) => names !== null && !names.has(candidate))
      .map(({ release }) => release);
    if (lacking.length > 0) lookups.set(candidate, lacking);
  }
  return lookups;
}

/**
 * The libraries the executable may open at run time: every library name among
 * its string literals, and those in `required`. `dlopen` and `dlsym` leave no
 * other trace in a file, so this is a reading of strings: it cannot tell which
 * of the names the code does without.
 */
export function runtimeLibraries(
  elf: ElfFile,
  firmwares: readonly Firmware[],
  required: readonly string[],
): RuntimeLibrary[] {
  const strings = literalStrings(elf);
  const candidates = [...strings].filter((text) => SYMBOL_NAME.test(text));
  const found = [...strings].filter(
    (text) => LIBRARY_NAME.test(text) && !elf.needed.includes(text),
  );
  return [...new Set([...required, ...found])].sort().map((name) => ({
    name,
    required: required.includes(name),
    absentOn: firmwares
      .filter((firmware) => firmware.library(name) === null)
      .map((firmware) => firmware.release),
    lookups: required.includes(name) ? lookupsIn(name, candidates, firmwares) : new Map(),
  }));
}
