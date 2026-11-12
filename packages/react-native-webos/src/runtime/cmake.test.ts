import { mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { recordingTask } from '../fakes.fixtures.ts';
import { TOOLCHAIN_FILE } from '../package-paths.ts';
import { buildCMakeProject, sourcesKey, webosDefinitions } from './cmake.ts';

let dir: string;

beforeEach(() => {
  dir = mkdtempSync(join(tmpdir(), 'rn-webos-cmake-'));
});

afterEach(() => {
  rmSync(dir, { recursive: true, force: true });
});

function project(definitions: Record<string, string>) {
  return { source: '/src', build: join(dir, 'build'), definitions, targets: ['hermesc'] };
}

describe('buildCMakeProject', () => {
  it('configures with Ninja and the definitions, then builds the targets', async () => {
    const task = recordingTask();

    await buildCMakeProject(task, project({ A: '1' }));

    expect(task.commands).toEqual([
      ['cmake', '-S', '/src', '-B', join(dir, 'build'), '-G', 'Ninja', '-DA=1'],
      ['cmake', '--build', join(dir, 'build'), '--target', 'hermesc'],
    ]);
  });

  it('skips configuring when nothing changed since the last configure', async () => {
    await buildCMakeProject(recordingTask(), project({ A: '1' }));
    writeFileSync(join(dir, 'build', 'build.ninja'), '');
    const task = recordingTask();

    await buildCMakeProject(task, project({ A: '1' }));
    await buildCMakeProject(task, project({ A: '2' }));

    expect(task.commands.map((command) => command[1])).toEqual(['--build', '-S', '--build']);
  });

  it("reports ninja's progress", async () => {
    const task = recordingTask();
    task.run = async (command, options) => {
      options?.onLine?.('[5/20] Building CXX object a.o');
      return command.join(' ');
    };

    await buildCMakeProject(task, { ...project({}), targets: [] });

    expect(task.progressed).toContain('configuring');
    expect(task.progressed.length).toBeGreaterThan(1);
  });
});

describe('webosDefinitions', () => {
  it('cross-compiles through the toolchain file, against the SDK, with the clang it is given', () => {
    const sdk = { path: '/cache/sdk/2026.08-webos', version: '2026.08-webos', reused: true };

    expect(webosDefinitions(sdk, '/opt/homebrew/opt/llvm/bin')).toEqual({
      CMAKE_TOOLCHAIN_FILE: TOOLCHAIN_FILE,
      CMAKE_BUILD_TYPE: 'Release',
      WEBOS_SDK_DIR: '/cache/sdk/2026.08-webos',
      WEBOS_LLVM_DIR: '/opt/homebrew/opt/llvm/bin',
    });
  });
});

describe('sourcesKey', () => {
  it('is twelve hex digits that change with the paths', () => {
    expect(sourcesKey(dir)).toMatch(/^[0-9a-f]{12}$/);
    expect(sourcesKey(dir)).not.toBe(sourcesKey(tmpdir()));
  });
});
