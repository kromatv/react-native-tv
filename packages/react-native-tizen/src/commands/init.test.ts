import { mkdtempSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { ask, canAsk } from '../cli/ask.ts';
import { captureProcessOutput } from '../fakes.fixtures.ts';
import { initCommand } from './init.ts';

vi.mock('../cli/ask.ts', () => ({ ask: vi.fn(), canAsk: vi.fn(() => false) }));
vi.mock('../app/expo-config.ts', () => ({ readExpoConfig: vi.fn(() => ({ slug: 'tv-app' })) }));

let dir = '';
let output: ReturnType<typeof captureProcessOutput>;

beforeEach(() => {
  dir = mkdtempSync(join(tmpdir(), 'rn-tizen-init-'));
  writeFileSync(join(dir, 'package.json'), '{ "name": "tv-app" }');
  writeFileSync(join(dir, 'app.json'), '{ "expo": { "name": "TV", "slug": "tv-app" } }');
  output = captureProcessOutput();
});

afterEach(() => {
  vi.restoreAllMocks();
  vi.mocked(ask).mockReset();
  vi.mocked(canAsk).mockReset().mockReturnValue(false);
  rmSync(dir, { recursive: true, force: true });
});

const appJson = () => JSON.parse(readFileSync(join(dir, 'app.json'), 'utf8'));
const run = (...args: string[]) => initCommand.run(args, { cwd: dir });

describe('rn-tizen init', () => {
  it('lists the plugin with the package id given and writes the Metro config, then changes nothing', async () => {
    await run('--package', 'com.example.tv');
    const files = [
      readFileSync(join(dir, 'app.json'), 'utf8'),
      readFileSync(join(dir, 'metro.config.mjs'), 'utf8'),
    ];
    await run();

    expect(appJson().expo.plugins).toEqual([
      ['@kromatv/react-native-tizen', { package: 'com.example.tv' }],
    ]);
    expect([
      readFileSync(join(dir, 'app.json'), 'utf8'),
      readFileSync(join(dir, 'metro.config.mjs'), 'utf8'),
    ]).toEqual(files);
    expect(output.stdout()).toContain('added the @kromatv/react-native-tizen config plugin');
    expect(output.stdout()).toContain('metro.config.mjs written');
    expect(output.stdout()).toContain(
      'lists the @kromatv/react-native-tizen config plugin already',
    );
    expect(output.stdout()).toContain('metro.config.mjs uses withTizen already');
    expect(output.stdout()).toContain('shift+m');
  });

  it('takes the suggested id without a terminal, and says so', async () => {
    await run();

    expect(appJson().expo.plugins[0][1]).toEqual({ package: 'com.anonymous.tvapp' });
    expect(output.stdout()).toContain('Package id com.anonymous.tvapp; pass --package');
  });

  it('asks on a terminal until the id is valid', async () => {
    vi.mocked(canAsk).mockReturnValue(true);
    vi.mocked(ask).mockResolvedValueOnce('not valid').mockResolvedValueOnce('org.example.tv');

    await run();

    expect(ask).toHaveBeenCalledWith('Tizen package id', 'com.anonymous.tvapp');
    expect(ask).toHaveBeenCalledTimes(2);
    expect(appJson().expo.plugins[0][1]).toEqual({ package: 'org.example.tv' });
    expect(output.stdout()).toContain('not valid: expected a reverse-DNS id');
  });

  it('prints what to add to an app.config file and a Metro config it cannot change safely', async () => {
    writeFileSync(join(dir, 'metro.config.js'), 'module.exports = {\n  resolver: {},\n};\n');

    await run('--package', 'com.example.tv');

    expect(output.stdout()).toContain(
      'metro.config.js cannot be changed safely: add these two lines',
    );
    expect(output.stdout()).toContain(
      "const { withTizen } = require('@kromatv/react-native-tizen/metro');",
    );
  });

  it('prints the plugin entry to add to an app.config file', async () => {
    writeFileSync(join(dir, 'app.config.js'), 'module.exports = {};');

    await run();

    expect(output.stdout()).toContain(
      'app.config.js is evaluated by Expo: add this entry to its plugins',
    );
    expect(output.stdout()).toContain(
      '["@kromatv/react-native-tizen",{"package":"com.anonymous.tvapp"}]',
    );
  });

  it('refuses a bad package id and a directory that is not an app', async () => {
    await expect(run('--package', 'tv')).rejects.toThrow('tv is not a Tizen package id');
    rmSync(join(dir, 'package.json'));
    await expect(run('--package', 'com.example.tv')).rejects.toThrow('has no package.json');
  });
});
