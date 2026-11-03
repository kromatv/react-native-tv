import { existsSync, readFileSync } from 'node:fs';
import { homedir } from 'node:os';
import { join } from 'node:path';
import { z } from 'zod';

/** One certificate of a profile: its PKCS#12 file and the password field Tizen Studio wrote for it. */
export interface ProfileKey {
  readonly path: string;
  readonly password: string;
}

/** A signing profile: one author certificate and up to two distributor certificates. */
export interface SecurityProfile {
  readonly name: string;
  readonly author: ProfileKey | null;
  readonly distributors: readonly ProfileKey[];
}

export interface ProfilesFile {
  readonly active: string | null;
  readonly profiles: readonly SecurityProfile[];
}

const MAX_FILE_BYTES = 1024 * 1024;
const ENTITIES: Record<string, string> = {
  '&amp;': '&',
  '&lt;': '<',
  '&gt;': '>',
  '&quot;': '"',
  '&apos;': "'",
};

const itemSchema = z.object({
  distributor: z.enum(['0', '1', '2']),
  key: z.string().max(4096),
  password: z.string().max(4096),
});
const profileSchema = z.object({ name: z.string().min(1).max(256) });
const rootSchema = z.object({ active: z.string().max(256).optional() });

function attributes(text: string): Record<string, string> {
  const values: Record<string, string> = {};
  let rest = text;
  for (let at = rest.indexOf('="'); at !== -1; at = rest.indexOf('="')) {
    const end = rest.indexOf('"', at + 2);
    if (end === -1) break;
    const name = rest.slice(0, at).trim().split(/\s+/).at(-1) ?? '';
    const value = rest.slice(at + 2, end);
    values[name] = value.replace(
      /&(?:amp|lt|gt|quot|apos);/g,
      (entity) => ENTITIES[entity] ?? entity,
    );
    rest = rest.slice(end + 1);
  }
  return values;
}

interface Tag {
  readonly closing: boolean;
  readonly tag: string;
  readonly body: string;
}

function profileTags(xml: string): Tag[] {
  const tags: Tag[] = [];
  for (let start = xml.indexOf('<'); start !== -1; start = xml.indexOf('<', start + 1)) {
    const end = xml.indexOf('>', start);
    if (end === -1) break;
    const inside = xml.slice(start + 1, end);
    const closing = inside.startsWith('/');
    const text = closing ? inside.slice(1) : inside;
    const space = text.search(/[\s/]/);
    const tag = space === -1 ? text : text.slice(0, space);
    if (tag === 'profiles' || tag === 'profile' || tag === 'profileitem') {
      tags.push({ closing, tag, body: space === -1 ? '' : text.slice(space) });
    }
  }
  return tags;
}

function profileFrom(name: string, items: z.infer<typeof itemSchema>[]): SecurityProfile {
  const key = (slot: string) => {
    const item = items.find((candidate) => candidate.distributor === slot && candidate.key !== '');
    return item === undefined ? null : { path: item.key, password: item.password };
  };
  const distributors = [key('1'), key('2')].filter((item) => item !== null);
  return { name, author: key('0'), distributors };
}

/** Reads Tizen Studio's `profiles.xml`: which profile is active, and each profile's certificates. */
export function parseProfiles(xml: string): ProfilesFile {
  let active: string | null = null;
  const profiles: SecurityProfile[] = [];
  let current: { name: string; items: z.infer<typeof itemSchema>[] } | null = null;
  for (const { closing, tag, body } of profileTags(xml)) {
    if (tag === 'profiles' && !closing) active = rootSchema.parse(attributes(body)).active ?? null;
    if (tag === 'profile' && !closing)
      current = { name: profileSchema.parse(attributes(body)).name, items: [] };
    if (tag === 'profile' && closing && current !== null) {
      profiles.push(profileFrom(current.name, current.items));
      current = null;
    }
    if (tag === 'profileitem' && current !== null)
      current.items.push(itemSchema.parse(attributes(body)));
  }
  return { active: active === '' ? null : active, profiles };
}

function dataDirFromSdkInfo(sdkDir: string): string | null {
  const info = join(sdkDir, 'sdk.info');
  if (!existsSync(info)) return null;
  const line = readFileSync(info, 'utf8')
    .split(/\r?\n/)
    .find((text) => text.startsWith('TIZEN_SDK_DATA_PATH='));
  const value = line?.slice('TIZEN_SDK_DATA_PATH='.length).trim();
  return value === undefined || value === '' ? null : value;
}

/**
 * Where Tizen Studio keeps its signing profiles: `TIZEN_SDK_DATA_DIR`, else the
 * data path its `sdk.info` names, else `~/tizen-studio-data`.
 */
export function profilesPath(env: NodeJS.ProcessEnv = process.env): string {
  const sdkDir = env.TIZEN_SDK_DIR ?? join(homedir(), 'tizen-studio');
  const dataDir =
    env.TIZEN_SDK_DATA_DIR ?? dataDirFromSdkInfo(sdkDir) ?? join(homedir(), 'tizen-studio-data');
  return join(dataDir, 'profile', 'profiles.xml');
}

/** The profiles file at `path`, or none when it does not exist. */
export function readProfiles(path = profilesPath()): ProfilesFile {
  if (!existsSync(path)) return { active: null, profiles: [] };
  const xml = readFileSync(path, 'utf8');
  if (xml.length > MAX_FILE_BYTES) throw new Error(`${path} is larger than a profiles file can be`);
  return parseProfiles(xml);
}
