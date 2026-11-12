import { mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, describe, expect, it } from 'vitest';
import { readFileSync } from 'node:fs';
import { objectAt, readJsonObject, writeJsonObject } from './json-file.ts';

const dirs: string[] = [];

afterEach(() => {
  for (const dir of dirs.splice(0)) rmSync(dir, { recursive: true, force: true });
});

function file(text: string): string {
  const dir = mkdtempSync(join(tmpdir(), 'rn-webos-json-'));
  dirs.push(dir);
  writeFileSync(join(dir, 'package.json'), text);
  return join(dir, 'package.json');
}

describe('readJsonObject', () => {
  it('refuses a file too large to be a manifest', () => {
    expect(() => readJsonObject(file(' '.repeat(4 * 1024 * 1024 + 1)))).toThrow('is too large');
  });
});

describe('writeJsonObject', () => {
  it('keeps the escapes of a file that wrote no character outside ASCII', () => {
    const path = file('{\n\t"done": "\\u2713 ready \\u2192 dist"\n}\n');

    writeJsonObject(path, { ...readJsonObject(path), added: true });

    expect(readFileSync(path, 'utf8')).toBe(
      '{\n\t"done": "\\u2713 ready \\u2192 dist",\n\t"added": true\n}\n',
    );
  });

  it('leaves the characters of a file that writes them as they are', () => {
    const path = file('{\n  "done": "✓ ready"\n}\n');

    writeJsonObject(path, { ...readJsonObject(path), next: '→' });

    expect(readFileSync(path, 'utf8')).toBe('{\n  "done": "✓ ready",\n  "next": "→"\n}\n');
  });
});

describe('objectAt', () => {
  it('gives an empty object for a key that is missing or not an object', () => {
    expect(objectAt({ a: 'text' }, 'a')).toEqual({});
    expect(objectAt({}, 'a')).toEqual({});
    expect(objectAt({ a: { b: 1 } }, 'a')).toEqual({ b: 1 });
  });
});
