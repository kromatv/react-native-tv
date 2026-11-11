import { mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, describe, expect, it, vi } from 'vitest';
import { fakeApp } from '../app/app.fixtures.ts';
import { loadAppProject } from '../app/app-project.ts';
import { targetPackage } from './package-option.ts';

vi.mock('../app/app-project.ts', () => ({ loadAppProject: vi.fn() }));

const dirs: string[] = [];

afterEach(() => {
  vi.mocked(loadAppProject).mockReset();
  for (const dir of dirs.splice(0)) rmSync(dir, { recursive: true, force: true });
});

function dir(withConfig: boolean): string {
  const path = mkdtempSync(join(tmpdir(), 'rn-tizen-package-option-'));
  dirs.push(path);
  if (withConfig) writeFileSync(join(path, 'app.json'), '{}');
  return path;
}

describe('targetPackage', () => {
  it('takes --package, else the app in the directory, else none', () => {
    const app = dir(true);
    vi.mocked(loadAppProject).mockReturnValue(fakeApp(app));

    expect(targetPackage('com.example.tv', app, 'perf')).toBe('com.example.tv');
    expect(targetPackage(undefined, app, 'perf')).toBe(fakeApp(app).config.packageId);
    expect(targetPackage(undefined, dir(false), 'perf')).toBeNull();
  });

  it('takes none from an app it cannot read, and refuses a bad --package', () => {
    vi.mocked(loadAppProject).mockImplementation(() => {
      throw new Error('no Tizen settings');
    });

    expect(targetPackage(undefined, dir(true), 'perf')).toBeNull();
    expect(() => targetPackage('tv', dir(false), 'perf')).toThrow(
      '--package is not a Tizen package id',
    );
  });
});
