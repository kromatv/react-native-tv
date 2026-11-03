import { mkdirSync, mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { dirname, join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { fakeApp } from '../app/app.fixtures.ts';
import { recordingTask } from '../fakes.fixtures.ts';
import { platformProfile } from '../tizen/platform-profiles.ts';
import { buildDotnetProject } from './dotnet-build.ts';

let dir: string;

beforeEach(() => {
  dir = mkdtempSync(join(tmpdir(), 'rn-tizen-dotnet-'));
});

afterEach(() => {
  rmSync(dir, { recursive: true, force: true });
});

describe('buildDotnetProject', () => {
  it('builds the shell in Release and returns the .tpk it wrote, an earlier one removed first', async () => {
    const { config } = fakeApp(dir);
    const profile = platformProfile('9.0');
    const tpk = join(dir, 'bin', 'Release', profile.targetFramework, 'org.example.demo-1.0.0.tpk');
    mkdirSync(dirname(tpk), { recursive: true });
    writeFileSync(tpk, 'stale');
    const task = recordingTask(() => {
      writeFileSync(tpk, 'fresh');
      return '';
    });

    expect(await buildDotnetProject(task, dir, config, profile)).toBe(tpk);
    expect(task.commands).toEqual([['dotnet', 'build', '-c', 'Release', '-nologo']]);
    expect(task.options[0]).toEqual({ cwd: dir });
  });

  it('fails when the build says it succeeded but wrote no package', async () => {
    const { config } = fakeApp(dir);

    await expect(
      buildDotnetProject(recordingTask(), dir, config, platformProfile('9.0')),
    ).rejects.toThrow('dotnet build reported success but wrote no');
  });
});
