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
  writeJson(join(app, 'app.json'), {
    expo: { name: 'Packed', slug: 'packed', tizen: { package: 'org.example.packed' } },
  });
  writeFileSync(
    join(app, 'metro.config.js'),
    [
      "const { getDefaultConfig } = require('expo/metro-config');",
      "const { withTizen } = require('@kromatv/react-native-tizen/metro');",
      '',
      'module.exports = withTizen(getDefaultConfig(__dirname));',
      '',
    ].join('\n'),
  );
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
  await sh(['npx', 'rn-tizen', 'bundle'], app);
  if (!existsSync(join(app, BYTECODE))) throw new Error(`rn-tizen bundle wrote no ${BYTECODE}`);
  reporter.line('The packed package bundles a fresh Expo app under Node.');
} finally {
  rmSync(work, { recursive: true, force: true });
}
