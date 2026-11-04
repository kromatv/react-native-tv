import { mkdtempSync, readFileSync, rmSync, statSync, utimesSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { stageFile } from './stage-file.ts';

let root: string;

beforeEach(() => {
  root = mkdtempSync(join(tmpdir(), 'rn-tizen-stage-'));
});

afterEach(() => {
  rmSync(root, { recursive: true, force: true });
});

function file(name: string, text: string, modified: Date): string {
  const path = join(root, name);
  writeFileSync(path, text);
  utimesSync(path, modified, modified);
  return path;
}

const yearAgo = new Date(Math.floor(Date.now() / 1000 - 365 * 24 * 3600) * 1000);

describe('stageFile', () => {
  it('dates a new or changed copy now, whatever the time of its source', async () => {
    const source = file('release.so', 'release', yearAgo);
    const target = file('staged.so', 'development', new Date());
    const before = Date.now() - 1000;

    await stageFile(source, target);
    await stageFile(source, join(root, 'fresh.so'));

    expect(readFileSync(target, 'utf8')).toBe('release');
    expect(statSync(target).mtimeMs).toBeGreaterThan(before);
    expect(statSync(join(root, 'fresh.so')).mtimeMs).toBeGreaterThan(before);
  });

  it('leaves a copy that already holds the same bytes as it is', async () => {
    const source = file('runtime.so', 'same', new Date());
    const target = file('staged.so', 'same', yearAgo);

    await stageFile(source, target);

    expect(statSync(target).mtimeMs).toBe(yearAgo.getTime());
  });

  it('replaces a copy of the same size and other bytes', async () => {
    const source = file('runtime.so', 'aaaa', yearAgo);
    const target = file('staged.so', 'bbbb', yearAgo);

    await stageFile(source, target);

    expect(readFileSync(target, 'utf8')).toBe('aaaa');
  });
});
