import type { Firmware, FirmwareLibrary } from './firmware-data.ts';

interface FixtureLibrary {
  readonly needed?: readonly string[];
  readonly symbols: readonly string[];
}

/** A firmware image holding the given libraries, by SONAME. */
export function firmware(
  release: string,
  libraries: Readonly<Record<string, FixtureLibrary>>,
): Firmware {
  const loaded = new Map<string, FirmwareLibrary>(
    Object.entries(libraries).map(([name, { needed = [], symbols }]) => [
      name,
      {
        name,
        needed,
        symbols: new Set(symbols),
        names: new Set(symbols.map((symbol) => symbol.split('@', 1)[0] ?? symbol)),
      },
    ]),
  );
  return { release, otaId: `HE_DTV_TEST_${release}`, library: (name) => loaded.get(name) ?? null };
}
