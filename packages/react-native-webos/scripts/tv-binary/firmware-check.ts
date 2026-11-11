import type { ImportedSymbol } from './elf-file.ts';
import type { Firmware, FirmwareLibrary } from './firmware-data.ts';

/** What an executable asks of a TV: the libraries it names and the symbols it leaves to them. */
export interface BinaryNeeds {
  readonly needed: readonly string[];
  readonly imports: readonly ImportedSymbol[];
  /** Libraries the code opens with `dlopen` and cannot run without. */
  readonly opened: readonly string[];
}

/** What one firmware image lacks of an executable's needs. */
export interface FirmwareResult {
  readonly release: string;
  readonly otaId: string;
  /** Needed or opened libraries the image does not have. */
  readonly missingLibraries: readonly string[];
  /** Version nodes, as `library: version`, that the named library does not define. */
  readonly missingVersions: readonly string[];
  /** Imported symbols, as `name` or `name@version`, that no loaded library defines. */
  readonly missingSymbols: readonly string[];
  /** Weak imports nothing defines: null at run time, which the code must expect. */
  readonly missingWeakSymbols: readonly string[];
}

/** Every library the dynamic linker loads for `needed`, in load order: their dependencies come too. */
function loadedLibraries(firmware: Firmware, needed: readonly string[]): FirmwareLibrary[] {
  const seen = new Set<string>();
  const loaded: FirmwareLibrary[] = [];
  const queue = [...needed];
  for (let name = queue.shift(); name !== undefined; name = queue.shift()) {
    if (seen.has(name)) continue;
    seen.add(name);
    const library = firmware.library(name);
    if (library === null) continue;
    loaded.push(library);
    queue.push(...library.needed);
  }
  return loaded;
}

function defines(libraries: readonly FirmwareLibrary[], symbol: ImportedSymbol): boolean {
  if (symbol.version === null) return libraries.some((library) => library.names.has(symbol.name));
  const versioned = `${symbol.name}@${symbol.version}`;
  return libraries.some((library) => library.symbols.has(versioned));
}

/** Checks an executable's needs against one firmware image, as its dynamic linker would at start. */
export function checkFirmware(firmware: Firmware, needs: BinaryNeeds): FirmwareResult {
  const direct = [...needs.needed, ...needs.opened];
  const missingLibraries = direct.filter((name) => firmware.library(name) === null);
  const loaded = loadedLibraries(firmware, needs.needed);

  const versions = new Map<string, { library: string; version: string }>();
  for (const { library, version } of needs.imports) {
    if (library !== null && version !== null)
      versions.set(`${library}: ${version}`, { library, version });
  }
  // A version node is itself a symbol of the library that defines it.
  const missingVersions = [...versions.entries()]
    .filter(([, { library, version }]) => {
      const found = firmware.library(library);
      return found !== null && !found.symbols.has(`${version}@${version}`);
    })
    .map(([label]) => label);

  const label = (symbol: ImportedSymbol) =>
    symbol.version === null ? symbol.name : `${symbol.name}@${symbol.version}`;
  const undefinedSymbols = needs.imports.filter((symbol) => !defines(loaded, symbol));
  return {
    release: firmware.release,
    otaId: firmware.otaId,
    missingLibraries,
    missingVersions,
    missingSymbols: undefinedSymbols.filter((symbol) => !symbol.weak).map(label),
    missingWeakSymbols: undefinedSymbols.filter((symbol) => symbol.weak).map(label),
  };
}

/** True when the image has every library, version node and strong symbol the executable needs. */
export function isCompatible(result: FirmwareResult): boolean {
  return (
    result.missingLibraries.length === 0 &&
    result.missingVersions.length === 0 &&
    result.missingSymbols.length === 0
  );
}

/**
 * The oldest release from which every image is compatible, or null when the
 * newest one is not. `results` are ordered oldest first; an image that fails
 * between two that pass raises the floor above it.
 */
export function compatibilityFloor(results: readonly FirmwareResult[]): string | null {
  let floor: string | null = null;
  for (const result of results) {
    if (!isCompatible(result)) floor = null;
    else floor ??= result.release;
  }
  return floor;
}
