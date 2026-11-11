import { existsSync, readFileSync, writeFileSync } from 'node:fs';
import { join } from 'node:path';
import { z } from 'zod';
import { readExpoConfig } from '../app/expo-config.ts';
import { FixableError } from '../errors.ts';
import { isWebOSPluginEntry } from './plugin-entry.ts';
import { WEBOS_PLUGIN, webosPluginProps } from './webos-settings.ts';

/** What `addWebOSPlugin` did to the app's config. */
export type PluginSetup =
  | { readonly kind: 'added' | 'moved' | 'present'; readonly file: string }
  | { readonly kind: 'dynamic'; readonly file: string; readonly entry: string };

type JsonObject = Record<string, unknown>;
type ChoosePackageId = (expo: JsonObject) => Promise<string>;

const DYNAMIC_CONFIGS = ['js', 'cjs', 'mjs', 'ts', 'cts', 'mts'].map((ext) => `app.config.${ext}`);
const MAX_APP_JSON_BYTES = 1024 * 1024;
const jsonObjectSchema = z.record(z.string(), z.unknown());
const pluginsSchema = z.array(z.unknown());
const entrySchema = z.union([z.string(), z.tuple([z.string()]).rest(z.unknown())]);

function readAppJson(file: string): JsonObject {
  if (!existsSync(file)) return {};
  const text = readFileSync(file, 'utf8');
  if (text.length > MAX_APP_JSON_BYTES)
    throw new FixableError(`${file} is too large`, 'Fix app.json.');
  try {
    return jsonObjectSchema.parse(JSON.parse(text));
  } catch {
    throw new FixableError(
      `${file} is not a JSON object`,
      'Fix app.json, then run rn-webos init again.',
    );
  }
}

function indentOf(file: string): string {
  if (!existsSync(file)) return '  ';
  return /^([ \t]+)\S/m.exec(readFileSync(file, 'utf8'))?.[1] ?? '  ';
}

function webosEntryIndex(root: string, plugins: readonly unknown[]): number {
  return plugins.findIndex((entry) => {
    const parsed = entrySchema.safeParse(entry);
    if (!parsed.success) return false;
    const name = typeof parsed.data === 'string' ? parsed.data : parsed.data[0];
    return isWebOSPluginEntry(root, name);
  });
}

async function propsFor(expo: JsonObject, choose: ChoosePackageId): Promise<JsonObject> {
  if (expo.webos === undefined) return { package: await choose(expo) };
  const legacy = jsonObjectSchema.safeParse(expo.webos);
  if (!legacy.success) throw new FixableError('expo.webos is not an object', 'Fix app.json.');
  return legacy.data;
}

async function dynamicSetup(
  root: string,
  file: string,
  choose: ChoosePackageId,
): Promise<PluginSetup> {
  const expo = readExpoConfig(root);
  if (webosPluginProps(expo.plugins, (name) => isWebOSPluginEntry(root, name)) !== undefined) {
    return { kind: 'present', file };
  }
  const entry = JSON.stringify([WEBOS_PLUGIN, await propsFor(expo, choose)]);
  return { kind: 'dynamic', file, entry };
}

/**
 * Lists this package's config plugin in the app's `app.json`, with the
 * package id `choose` gives when it has none. A plugin already listed with
 * props is left as it is, and `expo.webos` of earlier releases moves into its
 * props. An `app.config.*` file, which Expo evaluates, is the user's to change.
 */
export async function addWebOSPlugin(root: string, choose: ChoosePackageId): Promise<PluginSetup> {
  const dynamic = DYNAMIC_CONFIGS.find((name) => existsSync(join(root, name)));
  if (dynamic !== undefined) return dynamicSetup(root, join(root, dynamic), choose);
  const file = join(root, 'app.json');
  const json = readAppJson(file);
  const wrapped = jsonObjectSchema.safeParse(json.expo);
  const expo = wrapped.success || !existsSync(file) ? (wrapped.data ?? {}) : json;
  const plugins = pluginsSchema.safeParse(expo.plugins ?? []);
  if (!plugins.success)
    throw new FixableError(`expo.plugins in ${file} is not a list`, 'Fix app.json.');
  const at = webosEntryIndex(root, plugins.data);
  if (at !== -1 && typeof plugins.data[at] !== 'string') return { kind: 'present', file };
  const { webos, ...rest } = expo;
  const entry = [at === -1 ? WEBOS_PLUGIN : plugins.data[at], await propsFor(expo, choose)];
  const listed =
    at === -1
      ? [...plugins.data, entry]
      : plugins.data.map((old, index) => (index === at ? entry : old));
  const config = { ...rest, plugins: listed };
  const next = expo === json ? config : { ...json, expo: config };
  writeFileSync(file, `${JSON.stringify(next, null, indentOf(file))}\n`);
  return { kind: webos === undefined ? 'added' : 'moved', file };
}
