import {
  existsSync,
  mkdirSync,
  mkdtempSync,
  readdirSync,
  readFileSync,
  rmSync,
  writeFileSync,
} from 'node:fs';
import { tmpdir } from 'node:os';
import { join, resolve } from 'node:path';
import { z } from 'zod';
import { Reporter } from '../src/cli/reporter.ts';

const REPO = resolve(import.meta.dirname, '..');
const BYTECODE = join('tizen', 'build', 'bundle', 'index.tizen.hbc');
const METRO = 'metro.config.mjs';
const LOAD_PLUGIN = `
import plugin from '@kromatv/react-native-tizen/app.plugin.js';
if (typeof plugin !== 'function') throw new Error('app.plugin.js exports no function');
const config = { name: 'Packed' };
if (plugin(config, { package: 'org.example.packed' }) !== config) throw new Error('the plugin changed the config');
let refused = false;
try {
  plugin(config, { package: 'packed' });
} catch {
  refused = true;
}
if (!refused) throw new Error('the plugin took a bad package id');
`;

const exampleSchema = z.object({
  dependencies: z.object({ expo: z.string(), react: z.string(), 'react-native': z.string() }),
});

const reporter = new Reporter({ verbose: true });

async function sh(command: readonly string[], cwd: string): Promise<void> {
  await reporter.step(command.slice(0, 3).join(' '), (task) => task.run(command, { cwd }));
}

function writeJson(path: string, value: unknown): void {
  writeFileSync(path, `${JSON.stringify(value, null, 2)}\n`);
}

function writeApp(app: string, tarball: string): void {
  const example = exampleSchema.parse(
    JSON.parse(readFileSync(join(REPO, 'example', 'package.json'), 'utf8')),
  );
  writeJson(join(app, 'package.json'), {
    name: 'packed-install-check',
    version: '1.0.0',
    private: true,
    main: 'index.js',
    dependencies: {
      ...example.dependencies,
      '@kromatv/react-native-tizen': `file:${tarball}`,
    },
  });
  writeJson(join(app, 'app.json'), { expo: { name: 'Packed', slug: 'packed' } });
  writeFileSync(
    join(app, 'index.js'),
    [
      "import { createElement } from 'react';",
      "import { Text } from 'react-native';",
      "import { registerRootComponent } from 'expo';",
      '',
      "registerRootComponent(() => createElement(Text, null, 'Packed'));",
      '',
    ].join('\n'),
  );
}

const work = mkdtempSync(join(tmpdir(), 'rn-tizen-packed-'));
try {
  const packs = join(work, 'pack');
  mkdirSync(packs);
  await sh(['npm', 'pack', '--pack-destination', packs], REPO);
  const tarball = readdirSync(packs).find((name) => name.endsWith('.tgz'));
  if (tarball === undefined) throw new Error('npm pack wrote no tarball');
  const app = join(work, 'app');
  mkdirSync(app);
  writeApp(app, join(packs, tarball));
  await sh(['npm', 'install', '--no-audit', '--no-fund', '--loglevel=error'], app);
  await sh(['npx', 'rn-tizen', '--version'], app);
  await sh(['npx', 'rn-tizen', 'init', '--package', 'org.example.packed'], app);
  const firstInit = [
    readFileSync(join(app, 'app.json'), 'utf8'),
    readFileSync(join(app, METRO), 'utf8'),
  ];
  await sh(['npx', 'rn-tizen', 'init'], app);
  const secondInit = [
    readFileSync(join(app, 'app.json'), 'utf8'),
    readFileSync(join(app, METRO), 'utf8'),
  ];
  if (secondInit.join() !== firstInit.join())
    throw new Error('rn-tizen init changed what it wrote before');
  await sh(['node', '--input-type=module', '--eval', LOAD_PLUGIN], app);
  await sh(['npx', 'expo', 'config', '--json'], app);
  await sh(['npx', 'rn-tizen', 'bundle'], app);
  if (!existsSync(join(app, BYTECODE))) throw new Error(`rn-tizen bundle wrote no ${BYTECODE}`);
  reporter.line(
    'rn-tizen init sets a fresh Expo app up, plain Node and Expo load the packed config plugin, and the package bundles the app.',
  );
} finally {
  rmSync(work, { recursive: true, force: true });
}
