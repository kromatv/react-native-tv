import { mkdirSync, mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { recordingTask } from '../fakes.fixtures.ts';
import { buildCMakeProject } from './cmake.ts';
import { pinnedDependency } from './dependencies.ts';
import { downloadVerified } from './download.ts';
import { ensureHermesHostCompilers, hermesHostCompilers } from './hermes-host.ts';

vi.mock('./download.ts', () => ({ downloadVerified: vi.fn() }));
vi.mock('./cmake.ts', () => ({ buildCMakeProject: vi.fn() }));

let cache: string;

beforeEach(() => {
  cache = mkdtempSync(join(tmpdir(), 'rn-tizen-hermes-'));
  vi.stubEnv('RN_TIZEN_CACHE_DIR', cache);
});

afterEach(() => {
  rmSync(cache, { recursive: true, force: true });
  vi.clearAllMocks();
  vi.unstubAllEnvs();
});

describe('ensureHermesHostCompilers', () => {
  it('downloads, extracts and builds hermesc and shermes once', async () => {
    const task = recordingTask();

    const built = await ensureHermesHostCompilers(task);

    expect(built.version).toBe(pinnedDependency('hermes').version);
    expect(downloadVerified).toHaveBeenCalled();
    expect(task.commands[0]?.slice(0, 2)).toEqual(['tar', '-xzf']);
    expect(buildCMakeProject).toHaveBeenCalledWith(
      task,
      expect.objectContaining({ targets: ['hermesc', 'shermes'] }),
    );
    expect(task.progressed).toEqual(['downloading', 'extracting']);
  });

  it('builds from the source it extracted before', async () => {
    const { version } = pinnedDependency('hermes');
    const source = join(cache, 'hermes', version, 'source');
    mkdirSync(source, { recursive: true });
    writeFileSync(join(source, 'CMakeLists.txt'), '');

    await ensureHermesHostCompilers(recordingTask());

    expect(downloadVerified).not.toHaveBeenCalled();
    expect(buildCMakeProject).toHaveBeenCalled();
  });

  it('does nothing once the compilers are built', async () => {
    const { importFile, hermesc } = hermesHostCompilers();
    mkdirSync(join(hermesc, '..'), { recursive: true });
    writeFileSync(importFile, '');
    writeFileSync(hermesc, '');

    const built = await ensureHermesHostCompilers(recordingTask());

    expect(hermesHostCompilers().built).toBe(true);
    expect(built.hermesc).toBe(hermesc);
    expect(buildCMakeProject).not.toHaveBeenCalled();
  });
});
