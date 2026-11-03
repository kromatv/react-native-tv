import { mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { fakeApp } from '../app/app.fixtures.ts';
import { recordingTask } from '../fakes.fixtures.ts';
import { guestRun } from './lima.ts';
import {
  appRuntimeKey,
  buildSimulatorRuntime,
  cleanSimulatorRuntimes,
} from './simulator-runtime.ts';

vi.mock('./lima.ts', () => ({ guestRun: vi.fn(async () => '') }));
vi.mock('../runtime/build-runtime.ts', () => ({ ensureCodegen: vi.fn(async () => false) }));
vi.mock('../libraries/prepare-native-libraries.ts', () => ({
  prepareNativeLibraries: vi.fn(async () => ({
    file: '/libraries.json',
    libraries: [],
    warnings: [],
  })),
}));

let root: string;

beforeEach(() => {
  root = mkdtempSync(join(tmpdir(), 'rn-tizen-sim-runtime-'));
  writeFileSync(join(root, 'package.json'), '{"name":"app"}');
  vi.stubEnv('RN_TIZEN_CACHE_DIR', join(root, 'cache'));
});

afterEach(() => {
  rmSync(root, { recursive: true, force: true });
  vi.clearAllMocks();
  vi.unstubAllEnvs();
});

describe('buildSimulatorRuntime', () => {
  it("builds in the VM under the app's runtime key and reports ninja's progress", async () => {
    const task = recordingTask();
    const app = fakeApp(root);
    const onWait = vi.fn();
    vi.mocked(guestRun).mockImplementation(async (_task, _command, _env, onLine) => {
      onLine?.('Waiting for another simulator build to finish...');
      onLine?.('[3/10] Building CXX object x.o');
      return '';
    });

    const key = await buildSimulatorRuntime(task, app, onWait);

    expect(key).toBe(appRuntimeKey(app));
    expect(onWait).toHaveBeenCalledTimes(1);
    expect(task.progressed).toEqual(
      expect.arrayContaining(['codegen', 'compiling', 'waiting for another build']),
    );
    expect(guestRun).toHaveBeenCalledWith(
      task,
      ['bash', expect.stringMatching(/guest\/build\.sh$/)],
      expect.objectContaining({
        RN_TIZEN_SIMULATOR_RUNTIME: key,
        RN_TIZEN_LIBRARIES_FILE: '/libraries.json',
      }),
      expect.any(Function),
    );
  });
});

describe('cleanSimulatorRuntimes', () => {
  it('lists the runtime keys the guest removed and nothing else it printed', async () => {
    vi.mocked(guestRun).mockResolvedValue('removing\n0123456789ab\nfedcba987654\n');

    expect(await cleanSimulatorRuntimes(recordingTask())).toEqual(['0123456789ab', 'fedcba987654']);
  });
});
