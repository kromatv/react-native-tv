import { mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, describe, expect, it } from 'vitest';
import { objectAt, readJsonObject } from './json-file.ts';

const dirs: string[] = [];

afterEach(() => {
  for (const dir of dirs.splice(0)) rmSync(dir, { recursive: true, force: true });
});

function file(text: string): string {
  const dir = mkdtempSync(join(tmpdir(), 'rn-tizen-json-'));
  dirs.push(dir);
  writeFileSync(join(dir, 'package.json'), text);
  return join(dir, 'package.json');
}

describe('readJsonObject', () => {
  it('refuses a file too large to be a manifest', () => {
    expect(() => readJsonObject(file(' '.repeat(4 * 1024 * 1024 + 1)))).toThrow('is too large');
  });
});

describe('objectAt', () => {
  it('gives an empty object for a key that is missing or not an object', () => {
    expect(objectAt({ a: 'text' }, 'a')).toEqual({});
    expect(objectAt({}, 'a')).toEqual({});
    expect(objectAt({ a: { b: 1 } }, 'a')).toEqual({ b: 1 });
  });
});
