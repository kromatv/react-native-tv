import { mkdirSync, mkdtempSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { readExpoConfig } from '../app/expo-config.ts';
import { ask, canAsk } from '../cli/ask.ts';
import { run as runCommand } from '../exec.ts';
import { captureProcessOutput } from '../fakes.fixtures.ts';
import { initCommand } from './init.ts';

vi.mock('../cli/ask.ts', () => ({ ask: vi.fn(), canAsk: vi.fn(() => false) }));
vi.mock('../app/expo-config.ts', () => ({ readExpoConfig: vi.fn(() => ({ slug: 'tv-app' })) }));
vi.mock('../exec.ts', async (importOriginal) => ({
  ...(await importOriginal<typeof import('../exec.ts')>()),
  run: vi.fn(async () => ''),
  runSync: vi.fn(() => '12.16.1\n'),
}));

const TVOS = 'npm:react-native-tvos@0.88.0-0';

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
  vi.mocked(runCommand).mockReset().mockResolvedValue('');
  rmSync(dir, { recursive: true, force: true });
});

const appJson = () => JSON.parse(readFileSync(join(dir, 'app.json'), 'utf8'));
const packageJson = () => JSON.parse(readFileSync(join(dir, 'package.json'), 'utf8'));

function installPackage(path: string, manifest: Record<string, unknown>): void {
  mkdirSync(join(dir, path), { recursive: true });
  writeFileSync(join(dir, path, 'package.json'), JSON.stringify(manifest));
}

function installTwoCopies(): void {
  installPackage('node_modules/react-native', { name: 'react-native-tvos', version: '0.88.0-0' });
  installPackage('node_modules/lib', { name: 'lib', dependencies: { 'react-native': '0.88.0' } });
  installPackage('node_modules/lib/node_modules/react-native', {
    name: 'react-native',
    version: '0.88.0',
  });
}
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

  it("gives the commands to run with the app's package manager", async () => {
    writeFileSync(join(dir, 'bun.lock'), '');

    await run('--package', 'com.example.tv');

    expect(output.stdout()).toContain('Next: bunx expo start');
    expect(output.stdout()).toContain('bunx rn-tizen doctor checks it');
  });

  it("names the rootstrap package the app's floor needs", async () => {
    await run('--package', 'com.example.tv');
    vi.mocked(readExpoConfig).mockReturnValueOnce({
      name: 'TV',
      plugins: [['@kromatv/react-native-tizen', { package: 'com.example.tv', platform: '8.0' }]],
    });
    await run();

    expect(output.stdout()).toContain(
      "Builds for Tizen 6.0, the app's floor, need IOT-Headed-6.0-NativeAppDevelopment-CLI",
    );
    expect(output.stdout()).toContain('need TIZEN-8.0-NativeAppDevelopment-CLI');
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

  it('points react-native at react-native-tvos, overrides it and installs, then leaves it', async () => {
    await run('--package', 'com.example.tv');
    await run();

    expect(packageJson()).toEqual({
      name: 'tv-app',
      dependencies: { 'react-native': TVOS },
      overrides: { 'react-native': '$react-native' },
    });
    expect(runCommand).toHaveBeenCalledOnce();
    expect(runCommand).toHaveBeenCalledWith(['npm', 'install'], {
      cwd: dir,
      quiet: true,
    });
    expect(output.stdout()).toContain(`package.json now names ${TVOS}`);
    expect(output.stdout()).toContain('package.json overrides react-native already');
  });

  it('says what to run with --no-install, and runs nothing', async () => {
    await run('--package', 'com.example.tv', '--no-install');

    expect(runCommand).not.toHaveBeenCalled();
    expect(output.stdout()).toContain('Run npm install to apply it.');
  });

  it("gives the lines to add to a workspace's inline pnpm overrides, and where to install", async () => {
    const app = join(dir, 'apps', 'tv');
    mkdirSync(app, { recursive: true });
    writeFileSync(join(app, 'package.json'), '{ "name": "tv" }');
    writeFileSync(join(app, 'app.json'), '{ "expo": { "name": "TV", "slug": "tv" } }');
    writeFileSync(join(dir, 'pnpm-lock.yaml'), '');
    writeFileSync(join(dir, 'pnpm-workspace.yaml'), 'overrides: { foo: 1.0.0 }\n');

    await initCommand.run(['--package', 'com.example.tv', '--no-install'], { cwd: app });

    expect(output.stdout()).toContain(
      'pnpm-workspace.yaml cannot be changed safely: add these lines',
    );
    expect(output.stdout()).toContain(`  react-native: '${TVOS}'`);
    expect(output.stdout()).toContain(`Run pnpm install in ${dir} to apply it.`);
  });

  it('installs again when the app loads two copies, and fails when one is left', async () => {
    writeFileSync(
      join(dir, 'package.json'),
      JSON.stringify({
        name: 'tv-app',
        dependencies: { 'react-native': TVOS, lib: '1.0.0' },
        overrides: { 'react-native': '$react-native' },
      }),
    );
    installTwoCopies();
    vi.mocked(runCommand).mockImplementationOnce(async () => {
      rmSync(join(dir, 'node_modules/lib/node_modules'), { recursive: true });
      return '';
    });

    await run('--package', 'com.example.tv');
    installTwoCopies();

    expect(runCommand).toHaveBeenCalledOnce();
    await expect(run()).rejects.toThrow(
      expect.objectContaining({
        message: expect.stringContaining('The app loads 2 copies of react-native'),
        hint: 'Delete node_modules, then run npm install again.',
      }),
    );
  });

  it('refuses a bad package id and a directory that is not an app', async () => {
    await expect(run('--package', 'tv')).rejects.toThrow('tv is not a Tizen package id');
    rmSync(join(dir, 'package.json'));
    await expect(run('--package', 'com.example.tv')).rejects.toThrow('has no package.json');
  });
});
