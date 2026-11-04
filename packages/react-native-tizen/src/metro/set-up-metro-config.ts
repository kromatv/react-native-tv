import { existsSync, readFileSync, writeFileSync } from 'node:fs';
import { dirname, extname, join } from 'node:path';
import { z } from 'zod';

/** What `setUpMetroConfig` did: wrote or wrapped a config, found one ready, or left the change to the user. */
export type MetroSetup =
  | { readonly kind: 'written' | 'wrapped' | 'present'; readonly file: string }
  | { readonly kind: 'manual'; readonly file: string; readonly lines: readonly string[] };

const METRO_IMPORT = "import { withTizen } from '@kromatv/react-native-tizen/metro';";
const METRO_REQUIRE = "const { withTizen } = require('@kromatv/react-native-tizen/metro');";
const NEW_CONFIG = [
  "import { getDefaultConfig } from 'expo/metro-config';",
  METRO_IMPORT,
  'export default withTizen(getDefaultConfig(import.meta.dirname));',
  '',
].join('\n');
const EXTENSIONS = ['.js', '.cjs', '.mjs', '.json', '.ts', '.cts', '.mts'];
const CONFIG_STEMS = ['metro.config', join('.config', 'metro')];
const MAX_CONFIG_BYTES = 256 * 1024;
const packageMetroSchema = z.looseObject({ metro: z.unknown().optional() });
const workspacesSchema = z.looseObject({ workspaces: z.unknown().optional() });

const ESM_EXPORT = /^export default (.+?);?$/;
const CJS_EXPORT = /^module\.exports = (.+?);?$/;
const ESM_IMPORT_LINE = /^import\s.+['"];?$/;
const CJS_REQUIRE_LINE = /^(?:const|let|var)\s[^=]+=\s*require\(['"][^'"]+['"]\)/;
const USE_STRICT = /^['"]use strict['"];?$/;
const NOT_A_CONFIG_VALUE = /=>|\bfunction\b|\bclass\b/;

function lastIndexWhere(lines: readonly string[], test: (line: string) => boolean): number {
  for (let index = lines.length - 1; index >= 0; index--)
    if (test(lines[index] ?? '')) return index;
  return -1;
}

function configIn(dir: string): string | null {
  for (const stem of CONFIG_STEMS) {
    for (const ext of EXTENSIONS) {
      const file = join(dir, `${stem}${ext}`);
      if (existsSync(file)) return file;
    }
  }
  return null;
}

function packageJson(dir: string): unknown {
  const file = join(dir, 'package.json');
  if (!existsSync(file)) return null;
  try {
    return JSON.parse(readFileSync(file, 'utf8'));
  } catch {
    return null;
  }
}

function packageJsonConfig(root: string): string | null {
  const parsed = packageMetroSchema.safeParse(packageJson(root));
  return parsed.success && parsed.data.metro !== undefined ? join(root, 'package.json') : null;
}

function isWorkspaceRoot(dir: string): boolean {
  const parsed = workspacesSchema.safeParse(packageJson(dir));
  return (
    (parsed.success && parsed.data.workspaces !== undefined) ||
    existsSync(join(dir, 'pnpm-workspace.yaml'))
  );
}

function workspaceRootAbove(dir: string): string | null {
  const parent = dirname(dir);
  if (parent === dir) return null;
  return isWorkspaceRoot(parent) ? parent : workspaceRootAbove(parent);
}

function configBetween(dir: string, stop: string): string | null {
  const parent = dirname(dir);
  const config = configIn(parent);
  if (config !== null || parent === stop || parent === dir) return config;
  return configBetween(parent, stop);
}

function workspaceConfig(root: string): string | null {
  const stop = workspaceRootAbove(root);
  return stop === null ? null : configBetween(root, stop);
}

function isModule(file: string, text: string): boolean {
  const ext = extname(file);
  if (ext === '.mjs' || ext === '.mts') return true;
  if (ext === '.cjs' || ext === '.cts') return false;
  return /^export default\b/m.test(text);
}

function exportLine(esm: boolean, value: string): string {
  return esm ? `export default withTizen(${value});` : `module.exports = withTizen(${value});`;
}

function isBalanced(value: string): boolean {
  const depth = (open: string, close: string) =>
    value.split(open).length - value.split(close).length;
  return depth('(', ')') === 0 && depth('{', '}') === 0 && depth('[', ']') === 0;
}

function exportedValue(text: string, esm: boolean): string {
  const exported = text.split('\n').find((line) => (esm ? ESM_EXPORT : CJS_EXPORT).test(line));
  const value = (esm ? ESM_EXPORT : CJS_EXPORT).exec(exported ?? '')?.[1]?.trim() ?? '';
  return value !== '' && isBalanced(value) ? value : 'config';
}

function manual(file: string): MetroSetup {
  const text = file.endsWith('.json') ? '' : readFileSync(file, 'utf8').slice(0, MAX_CONFIG_BYTES);
  const esm = file.endsWith('.json') || isModule(file, text);
  const lines = [esm ? METRO_IMPORT : METRO_REQUIRE, exportLine(esm, exportedValue(text, esm))];
  return { kind: 'manual', file, lines };
}

function wrapped(lines: readonly string[], esm: boolean): string[] | null {
  const exportPattern = esm ? ESM_EXPORT : CJS_EXPORT;
  const exportLines = lines.flatMap((line, index) => (exportPattern.test(line) ? [index] : []));
  const last = lastIndexWhere(lines, (line) => line.trim() !== '');
  const [at] = exportLines;
  if (exportLines.length !== 1 || at !== last || at === undefined) return null;
  const value = exportPattern.exec(lines[at] ?? '')?.[1]?.trim() ?? '';
  if (value === '' || !isBalanced(value) || NOT_A_CONFIG_VALUE.test(value)) return null;
  const importLine = esm ? METRO_IMPORT : METRO_REQUIRE;
  const lastImport = lastIndexWhere(lines, (line) =>
    (esm ? ESM_IMPORT_LINE : CJS_REQUIRE_LINE).test(line),
  );
  const after = lastImport === -1 && USE_STRICT.test(lines[0] ?? '') ? 0 : lastImport;
  const body = lines.map((line, index) => (index === at ? exportLine(esm, value) : line));
  return [...body.slice(0, after + 1), importLine, ...body.slice(after + 1)];
}

/**
 * Makes the app's Metro config teach Metro the `tizen` platform. With none,
 * writes `metro.config.mjs`; with one that has a single-line export, wraps
 * that export with `withTizen`; otherwise returns the two lines to add.
 */
export function setUpMetroConfig(root: string): MetroSetup {
  const file = configIn(root) ?? packageJsonConfig(root);
  if (file === null) {
    const above = workspaceConfig(root);
    if (above !== null) return manual(above);
    const written = join(root, 'metro.config.mjs');
    writeFileSync(written, NEW_CONFIG);
    return { kind: 'written', file: written };
  }
  if (file.endsWith('.json')) return manual(file);
  const text = readFileSync(file, 'utf8');
  if (text.includes('withTizen')) return { kind: 'present', file };
  if (text.length > MAX_CONFIG_BYTES) return manual(file);
  const result = wrapped(text.split('\n'), isModule(file, text));
  if (result === null) return manual(file);
  writeFileSync(file, result.join('\n'));
  return { kind: 'wrapped', file };
}
