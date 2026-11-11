import { existsSync, mkdirSync, mkdtempSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { FixableError } from '../errors.ts';
import { claimWebOSDir } from './webos-dir.ts';

let root: string;
let dir: string;

beforeEach(() => {
  root = mkdtempSync(join(tmpdir(), 'rn-webos-dir-'));
  dir = join(root, 'webos');
});

afterEach(() => {
  rmSync(root, { recursive: true, force: true });
});

describe('claimWebOSDir', () => {
  it('creates webos/ ignoring itself', () => {
    claimWebOSDir(dir);

    expect(readFileSync(join(dir, '.gitignore'), 'utf8')).toBe('*\n');
  });

  it('takes an empty webos/', () => {
    mkdirSync(dir);

    claimWebOSDir(dir);

    expect(existsSync(join(dir, '.gitignore'))).toBe(true);
  });

  it('takes back the webos/ it generated before', () => {
    mkdirSync(dir);
    writeFileSync(join(dir, '.gitignore'), '*\n');
    writeFileSync(join(dir, 'ReactNativeWebOS.csproj'), '<Project />');

    expect(() => claimWebOSDir(dir)).not.toThrow();
  });

  it("refuses a webos/ that holds the app's own files", () => {
    mkdirSync(dir);
    writeFileSync(join(dir, 'config.xml'), '<widget />');

    expect(() => claimWebOSDir(dir)).toThrow(FixableError);
    expect(readFileSync(join(dir, 'config.xml'), 'utf8')).toBe('<widget />');
    expect(existsSync(join(dir, '.gitignore'))).toBe(false);
  });

  it('refuses a webos/ whose .gitignore keeps some of it in git', () => {
    mkdirSync(dir);
    writeFileSync(join(dir, '.gitignore'), 'build/\n');

    expect(() => claimWebOSDir(dir)).toThrow("is the app's own");
  });

  it('refuses a file named webos', () => {
    writeFileSync(dir, 'not a directory');

    expect(() => claimWebOSDir(dir)).toThrow(FixableError);
  });
});
