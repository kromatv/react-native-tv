import { copyFile, mkdir, readFile, writeFile } from 'node:fs/promises';
import { existsSync } from 'node:fs';
import { dirname } from 'node:path';
import { parseProfiles, type ProfileKey } from './profiles-file.ts';

/** A profile to add: an author and one or two distributors, each a `.p12` and its password field. */
export interface NewProfile {
  readonly name: string;
  readonly author: ProfileKey;
  readonly distributors: readonly ProfileKey[];
}

const EMPTY_FILE =
  '<?xml version="1.0" encoding="UTF-8" standalone="no"?>\n<profiles version="3.1">\n</profiles>\n';

const attribute = (value: string) =>
  value.replaceAll('&', '&amp;').replaceAll('<', '&lt;').replaceAll('"', '&quot;');

function profileBlock(profile: NewProfile): string {
  const item = (slot: number, key: ProfileKey | undefined) =>
    `<profileitem ca="" distributor="${slot}" key="${attribute(key?.path ?? '')}" password="${attribute(key?.password ?? '')}" rootca=""/>`;
  return [
    `<profile name="${attribute(profile.name)}">`,
    item(0, profile.author),
    item(1, profile.distributors[0]),
    item(2, profile.distributors[1]),
    '</profile>',
  ].join('\n');
}

function withActive(xml: string, name: string): string {
  const start = xml.indexOf('<profiles');
  const end = xml.indexOf('>', start);
  if (start === -1 || end === -1) throw new Error('profiles.xml has no <profiles> element');
  const tag = xml.slice(start, end);
  const value = `active="${attribute(name)}"`;
  const replaced = tag.includes('active="')
    ? tag.replace(/active="[^"]*"/, value)
    : `${tag} ${value}`;
  return `${xml.slice(0, start)}${replaced}${xml.slice(end)}`;
}

async function rewrite(path: string, change: (xml: string) => string): Promise<void> {
  const exists = existsSync(path);
  const xml = exists ? await readFile(path, 'utf8') : EMPTY_FILE;
  const changed = change(xml);
  parseProfiles(changed);
  await mkdir(dirname(path), { recursive: true });
  if (exists) await copyFile(path, `${path}.bak`);
  await writeFile(path, changed);
}

/** Makes `name` the active profile, keeping a copy of the file as it was in `profiles.xml.bak`. */
export function setActiveProfile(path: string, name: string): Promise<void> {
  return rewrite(path, (xml) => {
    if (!parseProfiles(xml).profiles.some((profile) => profile.name === name)) {
      throw new Error(`No signing profile named ${name} in ${path}`);
    }
    return withActive(xml, name);
  });
}

/** Adds a profile and makes it the active one, leaving every other profile as it was. */
export function addProfile(path: string, profile: NewProfile): Promise<void> {
  return rewrite(path, (xml) => {
    if (parseProfiles(xml).profiles.some((existing) => existing.name === profile.name)) {
      throw new Error(`${path} already has a profile named ${profile.name}`);
    }
    const close = xml.lastIndexOf('</profiles>');
    if (close === -1) throw new Error('profiles.xml has no </profiles>');
    return withActive(
      `${xml.slice(0, close)}${profileBlock(profile)}\n${xml.slice(close)}`,
      profile.name,
    );
  });
}
