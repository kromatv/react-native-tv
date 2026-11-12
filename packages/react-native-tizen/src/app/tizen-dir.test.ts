import { existsSync, mkdirSync, mkdtempSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { FixableError } from '../errors.ts';
import { claimTizenDir } from './tizen-dir.ts';

let root: string;
let dir: string;

beforeEach(() => {
  root = mkdtempSync(join(tmpdir(), 'rn-tizen-dir-'));
  dir = join(root, 'tizen');
});

afterEach(() => {
  rmSync(root, { recursive: true, force: true });
});

describe('claimTizenDir', () => {
  it('creates tizen/ ignoring itself', () => {
    claimTizenDir(dir);

    expect(readFileSync(join(dir, '.gitignore'), 'utf8')).toBe('*\n');
  });

  it('takes an empty tizen/', () => {
    mkdirSync(dir);

    claimTizenDir(dir);

    expect(existsSync(join(dir, '.gitignore'))).toBe(true);
  });

  it('takes back the tizen/ it generated before', () => {
    mkdirSync(dir);
    writeFileSync(join(dir, '.gitignore'), '*\n');
    writeFileSync(join(dir, 'ReactNativeTizen.csproj'), '<Project />');

    expect(() => claimTizenDir(dir)).not.toThrow();
  });

  it("refuses a tizen/ that holds the app's own files", () => {
    mkdirSync(dir);
    writeFileSync(join(dir, 'config.xml'), '<widget />');

    expect(() => claimTizenDir(dir)).toThrow(FixableError);
    expect(readFileSync(join(dir, 'config.xml'), 'utf8')).toBe('<widget />');
    expect(existsSync(join(dir, '.gitignore'))).toBe(false);
  });

  it('refuses a tizen/ whose .gitignore keeps some of it in git', () => {
    mkdirSync(dir);
    writeFileSync(join(dir, '.gitignore'), 'build/\n');

    expect(() => claimTizenDir(dir)).toThrow("is the app's own");
  });

  it('refuses a file named tizen', () => {
    writeFileSync(dir, 'not a directory');

    expect(() => claimTizenDir(dir)).toThrow(FixableError);
  });
});
