import type { ImportedSymbol } from './elf-file.ts';

/** The symbols an executable imports under one version node of one library. */
export interface VersionUse {
  readonly library: string;
  readonly version: string;
  readonly symbols: readonly string[];
}

/** Orders `GLIBC_2.9` before `GLIBC_2.12`, and `GLIBCXX_3.4.9` before `GLIBCXX_3.4.30`. */
export function compareVersionNodes(left: string, right: string): number {
  const numbers = (node: string) => (node.split('_').at(-1) ?? '').split('.').map(Number);
  const [a, b] = [numbers(left), numbers(right)];
  for (let index = 0; index < Math.max(a.length, b.length); index += 1) {
    const difference = (a[index] ?? 0) - (b[index] ?? 0);
    if (difference !== 0) return difference;
  }
  return 0;
}

/** Every version node the imports name, with the symbols that need it, highest version last. */
export function versionUses(imports: readonly ImportedSymbol[]): VersionUse[] {
  const uses = new Map<string, { library: string; version: string; symbols: string[] }>();
  for (const { name, version, library } of imports) {
    if (version === null || library === null) continue;
    const key = `${library}\0${version}`;
    const use = uses.get(key) ?? { library, version, symbols: [] };
    use.symbols.push(name);
    uses.set(key, use);
  }
  return [...uses.values()].sort(
    (left, right) =>
      compareVersionNodes(left.version, right.version) || left.library.localeCompare(right.library),
  );
}

/** The highest version node with the given prefix, such as `GLIBC_` or `GLIBCXX_`; null when none is imported. */
export function highestVersion(uses: readonly VersionUse[], prefix: string): string | null {
  const nodes = uses.map((use) => use.version).filter((version) => version.startsWith(prefix));
  nodes.sort(compareVersionNodes);
  return nodes.at(-1) ?? null;
}
