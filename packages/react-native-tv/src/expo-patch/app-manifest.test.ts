import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { testApp, type TestApp } from '../app.fixtures.ts';
import { readManifest } from './app-manifest.ts';
import { readText } from './read-text.ts';
import { join } from 'node:path';

let app: TestApp;

beforeEach(() => {
  app = testApp();
});

afterEach(() => {
  app.remove();
});

describe('readManifest', () => {
  it('gives the text and the string members of the fields the patch reads', () => {
    const text = '{ "scripts": { "start": "expo start", "odd": 1 }, "resolutions": { "a": "1" } }';
    app.write('package.json', text);

    expect(readManifest(join(app.root, 'package.json'))).toEqual({
      text,
      scripts: { start: 'expo start' },
      patchedDependencies: {},
      resolutions: { a: '1' },
    });
  });

  it('reads a field of another kind as empty', () => {
    app.write('package.json', { scripts: 'none', patchedDependencies: ['a'] });

    expect(readManifest(join(app.root, 'package.json'))).toMatchObject({
      scripts: {},
      patchedDependencies: {},
    });
  });

  it('gives null for no file, for one that does not parse and for one that is not an object', () => {
    expect(readManifest(join(app.root, 'package.json'))).toBeNull();
    app.write('package.json', '{ "scripts": ');
    expect(readManifest(join(app.root, 'package.json'))).toBeNull();
    app.write('package.json', '[]');
    expect(readManifest(join(app.root, 'package.json'))).toBeNull();
  });
});

describe('readText', () => {
  it('reads a file up to a size, and gives null for a larger one or none', () => {
    app.write('notes.txt', 'twelve bytes');

    expect(readText(join(app.root, 'notes.txt'), 12)).toBe('twelve bytes');
    expect(readText(join(app.root, 'notes.txt'), 11)).toBeNull();
    expect(readText(join(app.root, 'missing.txt'), 12)).toBeNull();
  });
});
