import { existsSync, mkdtempSync, readFileSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, describe, expect, it } from 'vitest';
import { addProfile, setActiveProfile } from './profiles-edit.ts';
import { readProfiles } from './profiles-file.ts';

const dirs: string[] = [];

function profilesFile(): string {
  const dir = mkdtempSync(join(tmpdir(), 'rn-tizen-edit-'));
  dirs.push(dir);
  return join(dir, 'profile', 'profiles.xml');
}

const key = (path: string) => ({ path, password: `${path}.pwd` });

afterEach(() => {
  for (const dir of dirs.splice(0)) rmSync(dir, { recursive: true, force: true });
});

describe('addProfile and setActiveProfile', () => {
  it('creates the file, adds profiles beside each other and switches the active one', async () => {
    const path = profilesFile();

    await addProfile(path, { name: 'TV', author: key('/a.p12'), distributors: [key('/d.p12')] });
    await addProfile(path, { name: 'Store', author: key('/b.p12'), distributors: [key('/e.p12')] });
    await setActiveProfile(path, 'TV');

    const file = readProfiles(path);
    expect(file.active).toBe('TV');
    expect(file.profiles.map((profile) => profile.name)).toEqual(['TV', 'Store']);
    expect(file.profiles[1]?.distributors).toEqual([key('/e.p12')]);
    expect(existsSync(`${path}.bak`)).toBe(true);
  });

  it('refuses a duplicate name and an unknown profile, leaving the file as it was', async () => {
    const path = profilesFile();
    await addProfile(path, { name: 'TV', author: key('/a.p12'), distributors: [key('/d.p12')] });
    const before = readFileSync(path, 'utf8');

    await expect(
      addProfile(path, { name: 'TV', author: key('/x.p12'), distributors: [] }),
    ).rejects.toThrow('already has');
    await expect(setActiveProfile(path, 'Nope')).rejects.toThrow('No signing profile named Nope');
    expect(readFileSync(path, 'utf8')).toBe(before);
  });
});
