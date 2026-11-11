import { mkdtempSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, describe, expect, it, vi } from 'vitest';
import { readExpoConfig } from '../app/expo-config.ts';
import { addTizenPlugin } from './add-tizen-plugin.ts';

vi.mock('../app/expo-config.ts', () => ({ readExpoConfig: vi.fn() }));

const dirs: string[] = [];

afterEach(() => {
  vi.clearAllMocks();
  for (const dir of dirs.splice(0)) rmSync(dir, { recursive: true, force: true });
});

function app(appJson?: string): string {
  const dir = mkdtempSync(join(tmpdir(), 'rn-tizen-plugin-'));
  dirs.push(dir);
  if (appJson !== undefined) writeFileSync(join(dir, 'app.json'), appJson);
  return dir;
}

const readAppJson = (dir: string): unknown =>
  JSON.parse(readFileSync(join(dir, 'app.json'), 'utf8'));
const choose = vi.fn(async () => 'com.example.tv');

describe('addTizenPlugin', () => {
  it('adds the plugin with the package id chosen, keeping the other plugins and the indent', async () => {
    const dir = app('{\n    "expo": { "name": "TV", "plugins": ["expo-video"] }\n}\n');

    const setup = await addTizenPlugin(dir, choose);

    expect(setup).toEqual({ kind: 'added', file: join(dir, 'app.json') });
    expect(readAppJson(dir)).toEqual({
      expo: {
        name: 'TV',
        plugins: ['expo-video', ['@kromatv/react-native-tizen', { package: 'com.example.tv' }]],
      },
    });
    expect(readFileSync(join(dir, 'app.json'), 'utf8')).toMatch(/^\{\n {4}"expo"/);
    expect(choose).toHaveBeenCalledWith({ name: 'TV', plugins: ['expo-video'] });
  });

  it('changes nothing the second time, nor for a plugin listed by path with props', async () => {
    const dir = app('{ "expo": { "name": "TV" } }');
    await addTizenPlugin(dir, choose);
    const once = readFileSync(join(dir, 'app.json'), 'utf8');

    expect(await addTizenPlugin(dir, choose)).toMatchObject({ kind: 'present' });
    expect(readFileSync(join(dir, 'app.json'), 'utf8')).toBe(once);
    expect(choose).toHaveBeenCalledOnce();
  });

  it('gives a plugin listed by name alone its props', async () => {
    const dir = app('{ "expo": { "name": "TV", "plugins": ["@kromatv/react-native-tizen"] } }');

    expect(await addTizenPlugin(dir, choose)).toMatchObject({ kind: 'added' });
    expect(readAppJson(dir)).toEqual({
      expo: {
        name: 'TV',
        plugins: [['@kromatv/react-native-tizen', { package: 'com.example.tv' }]],
      },
    });
  });

  it('moves expo.tizen of earlier releases into the props, without asking', async () => {
    const dir = app(
      '{ "expo": { "name": "TV", "tizen": { "package": "org.old.tv", "label": "Old" } } }',
    );

    expect(await addTizenPlugin(dir, choose)).toMatchObject({ kind: 'moved' });
    expect(readAppJson(dir)).toEqual({
      expo: {
        name: 'TV',
        plugins: [['@kromatv/react-native-tizen', { package: 'org.old.tv', label: 'Old' }]],
      },
    });
    expect(choose).not.toHaveBeenCalled();
  });

  it('writes an app.json when there is none, and edits one without the expo key', async () => {
    const none = app();
    const bare = app('{ "name": "Bare" }');

    await addTizenPlugin(none, choose);
    await addTizenPlugin(bare, choose);

    expect(readAppJson(none)).toEqual({
      expo: { plugins: [['@kromatv/react-native-tizen', { package: 'com.example.tv' }]] },
    });
    expect(readAppJson(bare)).toEqual({
      name: 'Bare',
      plugins: [['@kromatv/react-native-tizen', { package: 'com.example.tv' }]],
    });
  });

  it("leaves an app.config file to the user, unless Expo's config lists the plugin", async () => {
    const dir = app();
    writeFileSync(join(dir, 'app.config.ts'), 'export default {};');
    vi.mocked(readExpoConfig)
      .mockReturnValueOnce({ name: 'TV', slug: 'tv' })
      .mockReturnValueOnce({ plugins: [['@kromatv/react-native-tizen', { package: 'a.b' }]] });

    expect(await addTizenPlugin(dir, choose)).toEqual({
      kind: 'dynamic',
      file: join(dir, 'app.config.ts'),
      entry: '["@kromatv/react-native-tizen",{"package":"com.example.tv"}]',
    });
    expect(await addTizenPlugin(dir, choose)).toMatchObject({ kind: 'present' });
    expect(choose).toHaveBeenCalledWith({ name: 'TV', slug: 'tv' });
  });

  it('refuses an app.json it cannot read as a config', async () => {
    await expect(addTizenPlugin(app('[1, 2]'), choose)).rejects.toThrow('is not a JSON object');
    await expect(addTizenPlugin(app('{ "expo": { "plugins": 3 } }'), choose)).rejects.toThrow(
      'is not a list',
    );
    await expect(addTizenPlugin(app('{ "expo": { "tizen": 3 } }'), choose)).rejects.toThrow(
      'expo.tizen is not an object',
    );
    await expect(addTizenPlugin(app(' '.repeat(1024 * 1024 + 1)), choose)).rejects.toThrow(
      'is too large',
    );
  });
});
