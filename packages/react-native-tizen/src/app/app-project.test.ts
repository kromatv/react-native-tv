import { mkdtempSync, rmSync, symlinkSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { FixableError } from '../errors.ts';
import { PACKAGE_ROOT } from '../package-paths.ts';
import { loadAppProject } from './app-project.ts';
import { readExpoConfig } from './expo-config.ts';

const EXAMPLE = join(PACKAGE_ROOT, 'example');

let dir: string;

function app(appConfig: string): string {
  writeFileSync(join(dir, 'package.json'), '{"name":"app","main":"index.js"}');
  writeFileSync(join(dir, 'app.config.js'), appConfig);
  symlinkSync(join(EXAMPLE, 'node_modules'), join(dir, 'node_modules'));
  return dir;
}

beforeEach(() => {
  dir = mkdtempSync(join(tmpdir(), 'rn-tizen-app-'));
});

afterEach(() => {
  rmSync(dir, { recursive: true, force: true });
});

describe('loadAppProject', () => {
  it("reads the example's config, entry and native packages", () => {
    const project = loadAppProject(EXAMPLE);

    expect(project.config.packageId).toBe('org.rntizen.example');
    expect(project.entryFile).toBe(join(EXAMPLE, 'main.ts'));
    expect(project.packages.reactNative.name).toBe('react-native-tvos');
    expect(project.tizenDir).toBe(join(EXAMPLE, 'tizen'));
  });

  it('takes the entry it was given', () => {
    const entry = join(EXAMPLE, 'app.json');

    expect(loadAppProject(EXAMPLE, entry).entryFile).toBe(entry);
  });

  it('refuses an entry that does not exist', () => {
    expect(() => loadAppProject(EXAMPLE, join(EXAMPLE, 'nope.ts'))).toThrow('does not exist');
  });

  it('refuses a directory without package.json', () => {
    expect(() => loadAppProject(dir)).toThrow(FixableError);
  });

  it("gives the reason Expo could not read the app's config", () => {
    app("throw new Error('config exploded');");

    expect(() => loadAppProject(dir)).toThrow(/Could not read the app config: .*config exploded/s);
  });
});

describe('readExpoConfig', () => {
  it('evaluates app.config.js with Node, public fields only when asked', () => {
    app(
      "module.exports = { name: 'Dynamic', slug: 'dynamic', extra: { built: 1 + 1 }, tizen: { package: 'a.b' } };",
    );

    expect(readExpoConfig(dir)).toMatchObject({ name: 'Dynamic', extra: { built: 2 } });
    expect(readExpoConfig(dir, { isPublicConfig: true })).toMatchObject({ name: 'Dynamic' });
  });
});
