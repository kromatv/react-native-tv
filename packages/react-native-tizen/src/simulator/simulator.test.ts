import { mkdirSync, mkdtempSync, readFileSync, realpathSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { fakeApp } from '../app/app.fixtures.ts';
import { FixableError } from '../errors.ts';
import { recordingTask } from '../fakes.fixtures.ts';
import { PACKAGE_ROOT } from '../package-paths.ts';
import { simulatorInstance } from './instance.ts';
import * as lima from './lima.ts';
import {
  crashReport,
  ensureVm,
  hostAddress,
  openUrl,
  pressKeys,
  printLog,
  requireRunningVm,
  saveScreenshot,
  sendControl,
  sendLifecycle,
  startApp,
  stopSimulator,
} from './simulator.ts';

vi.mock('./lima.ts', () => ({
  addMount: vi.fn(),
  createInstance: vi.fn(),
  editInstance: vi.fn(),
  guestRun: vi.fn(async () => ''),
  guestSpawn: vi.fn(async () => undefined),
  instanceState: vi.fn(),
  startInstance: vi.fn(),
  stopInstance: vi.fn(),
}));

const instance = simulatorInstance('ts');
let dir: string;

function projectApp() {
  const root = join(dir, 'app');
  mkdirSync(root);
  writeFileSync(join(root, 'package.json'), '{"name":"app"}');
  return fakeApp(root);
}

beforeEach(() => {
  dir = realpathSync(mkdtempSync(join(PACKAGE_ROOT, 'node_modules', '.rn-tizen-simulator-test-')));
  vi.stubEnv('RN_TIZEN_CACHE_DIR', join(dir, 'cache'));
});

afterEach(() => {
  rmSync(dir, { recursive: true, force: true });
  vi.clearAllMocks();
  vi.unstubAllEnvs();
});

describe('ensureVm', () => {
  it('creates, starts and provisions a VM that does not exist', async () => {
    vi.mocked(lima.instanceState).mockResolvedValue(null);
    const task = recordingTask();

    expect(await ensureVm(task, projectApp())).toBe('created');
    expect(lima.createInstance).toHaveBeenCalledWith(
      task,
      expect.stringMatching(/lima\.yaml$/),
      expect.objectContaining({ CacheDir: join(dir, 'cache') }),
    );
    expect(lima.startInstance).toHaveBeenCalled();
    expect(lima.guestRun).toHaveBeenCalledWith(task, [
      'sudo',
      'bash',
      expect.stringMatching(/guest\/provision\.sh$/),
    ]);
  });

  it('only provisions a running VM that already mounts the project', async () => {
    vi.mocked(lima.instanceState).mockResolvedValue({
      status: 'Running',
      param: { CacheDir: join(dir, 'cache') },
      config: { mounts: [{ location: '/' }] },
    });

    expect(await ensureVm(recordingTask(), projectApp())).toBe('running');
    expect(lima.startInstance).not.toHaveBeenCalled();
    expect(lima.addMount).not.toHaveBeenCalled();
  });

  it('stops the VM to mount a project it does not mount yet, then starts it', async () => {
    vi.mocked(lima.instanceState).mockResolvedValue({
      status: 'Running',
      param: { CacheDir: '/old/cache', ProjectDir: '/old' },
      config: { mounts: [{ location: '/old' }] },
    });

    expect(await ensureVm(recordingTask(), projectApp())).toBe('started');
    expect(lima.stopInstance).toHaveBeenCalled();
    expect(lima.editInstance).toHaveBeenCalledWith(expect.anything(), {
      ProjectDir: '/old',
      CacheDir: join(dir, 'cache'),
    });
    expect(lima.addMount).toHaveBeenCalled();
    expect(lima.startInstance).toHaveBeenCalled();
  });

  it("mounts the app's native libraries with the rest", async () => {
    vi.mocked(lima.instanceState).mockResolvedValue(null);
    const app = projectApp();
    writeFileSync(join(app.root, 'package.json'), '{"name":"app","dependencies":{"svg":"1"}}');
    mkdirSync(join(app.root, 'node_modules', 'svg'), { recursive: true });
    writeFileSync(
      join(app.root, 'node_modules', 'svg', 'package.json'),
      JSON.stringify({
        name: 'svg',
        version: '1.0.0',
        codegenConfig: { name: 'svg', type: 'components', jsSrcsDir: 'src' },
      }),
    );

    await ensureVm(recordingTask(), app);

    expect(lima.createInstance).toHaveBeenCalledWith(
      expect.anything(),
      expect.anything(),
      expect.objectContaining({ ProjectDir: realpathSync(PACKAGE_ROOT) }),
    );
  });

  it('refuses to mount a directory that holds the home directory', async () => {
    const app = fakeApp(tmpdir());

    await expect(ensureVm(recordingTask(), app)).rejects.toBeInstanceOf(FixableError);
  });
});

describe('requireRunningVm', () => {
  it('says how to create a VM that does not exist', async () => {
    vi.mocked(lima.instanceState).mockResolvedValue(null);

    await expect(requireRunningVm(recordingTask())).rejects.toThrow(
      'The simulator VM does not exist yet',
    );
  });

  it('starts a stopped VM', async () => {
    vi.mocked(lima.instanceState).mockResolvedValue({ status: 'Stopped' });

    await requireRunningVm(recordingTask());

    expect(lima.startInstance).toHaveBeenCalled();
  });
});

describe('the guest scripts of an instance', () => {
  it("reads the app's process id from the last line run prints", async () => {
    vi.mocked(lima.guestRun).mockResolvedValue('starting\n4242\n');

    expect(await startApp(recordingTask(), instance)).toBe(4242);
    expect(lima.guestRun).toHaveBeenCalledWith(
      expect.anything(),
      ['bash', expect.stringMatching(/guest\/run\.sh$/)],
      expect.objectContaining({ RN_TIZEN_SIMULATOR_INSTANCE: 'ts' }),
    );
  });

  it("finds this Mac's address as the VM sees it", async () => {
    vi.mocked(lima.guestRun).mockResolvedValue('192.168.5.2 STREAM host.lima.internal\n');

    expect(await hostAddress(recordingTask())).toBe('192.168.5.2');
  });

  it('says how to check the VM network when it cannot', async () => {
    vi.mocked(lima.guestRun).mockResolvedValue('\n');

    await expect(hostAddress(recordingTask())).rejects.toBeInstanceOf(FixableError);
  });

  it('refuses a control argument holding a line break', async () => {
    await expect(sendControl(recordingTask(), instance, 'tile', 'a\nb')).rejects.toThrow(
      'The tile argument cannot hold a line break',
    );
    expect(lima.guestRun).not.toHaveBeenCalled();
  });

  it('sends a deep link as an open-url control', async () => {
    await openUrl(recordingTask(), instance, 'app://x');

    expect(lima.guestRun).toHaveBeenCalledWith(
      expect.anything(),
      ['bash', expect.stringMatching(/control\.sh$/), 'open-url', 'app://x'],
      expect.anything(),
    );
  });

  it('prints the log with the lines and follow flag asked for', async () => {
    await printLog(instance, { lines: 20, follow: true });

    expect(lima.guestSpawn).toHaveBeenCalledWith(
      ['bash', expect.stringMatching(/logs\.sh$/), '-n', '20', '-f'],
      expect.anything(),
    );
  });

  it('returns the crash report', async () => {
    vi.mocked(lima.guestRun).mockResolvedValue('#0 abort\n');

    expect(await crashReport(recordingTask(), instance)).toBe('#0 abort\n');
  });

  it('writes a screenshot through a partial file', async () => {
    vi.mocked(lima.guestSpawn).mockImplementation(async (_command, streams) => {
      if (typeof streams?.stdout === 'number') writeFileSync(streams.stdout, 'png');
    });
    const path = join(dir, 'shots', 'screen.png');

    expect(await saveScreenshot(instance, path)).toBe(path);
    expect(readFileSync(path, 'utf8')).toBe('png');
  });

  it('presses keys, sends a lifecycle event and stops through the guest scripts', async () => {
    const task = recordingTask();

    await pressKeys(task, instance, ['Down', 'Enter']);
    await sendLifecycle(task, instance, 'pause');
    await stopSimulator(task, instance);

    expect(vi.mocked(lima.guestRun).mock.calls.map(([, command]) => command.slice(1))).toEqual([
      [expect.stringMatching(/key\.sh$/), 'Down', 'Enter'],
      [expect.stringMatching(/control\.sh$/), 'pause'],
      [expect.stringMatching(/stop\.sh$/)],
    ]);
  });
});
