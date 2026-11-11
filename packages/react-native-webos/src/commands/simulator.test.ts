import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { captureProcessOutput } from '../fakes.fixtures.ts';
import * as lima from '../simulator/lima.ts';
import * as simulator from '../simulator/simulator.ts';
import * as runtimes from '../simulator/simulator-runtime.ts';
import * as viewer from '../simulator/viewer.ts';
import { simulatorCommand } from './simulator.ts';
import * as simulatorRun from './simulator-run.ts';
import * as status from './simulator-status.ts';

vi.mock('../simulator/lima.ts', () => ({ requireLima: vi.fn(), deleteInstance: vi.fn() }));
vi.mock('../simulator/simulator.ts', () => ({
  crashReport: vi.fn(async () => 'thread 1\n'),
  openUrl: vi.fn(),
  pressKeys: vi.fn(),
  printLog: vi.fn(),
  requireRunningVm: vi.fn(),
  saveScreenshot: vi.fn(async (_instance: unknown, path: string) => path),
  sendPointer: vi.fn(),
  sendControl: vi.fn(),
  sendLifecycle: vi.fn(),
  startApp: vi.fn(async () => 4242),
  stopSimulator: vi.fn(),
}));
vi.mock('../simulator/simulator-runtime.ts', () => ({
  cleanSimulatorRuntimes: vi.fn(async () => ['0123456789ab']),
}));
vi.mock('../simulator/viewer.ts', () => ({
  startViewer: vi.fn(async () => 'http://127.0.0.1:6080/'),
  openInBrowser: vi.fn(),
}));
vi.mock('./simulator-run.ts', () => ({ runInSimulator: vi.fn(), prepareSimulator: vi.fn() }));
vi.mock('./simulator-status.ts', () => ({ printSimulatorStatus: vi.fn() }));
vi.mock('../perf/watch-perf.ts', () => ({ watchPerf: vi.fn(async () => []) }));

const context = { cwd: '/app' };
let output: ReturnType<typeof captureProcessOutput>;

function simulate(...args: string[]): Promise<void> {
  return simulatorCommand.run(args, context);
}

beforeEach(() => {
  output = captureProcessOutput();
  vi.stubEnv('RN_WEBOS_SIMULATOR_INSTANCE', '');
});

afterEach(() => {
  vi.clearAllMocks();
  vi.unstubAllEnvs();
});

describe('rn-webos simulator', () => {
  it('asks for an action', async () => {
    await expect(simulate()).rejects.toThrow('simulator needs an action');
  });

  it('suggests the action a typo was meant to be', async () => {
    await expect(simulate('restrat')).rejects.toThrow(
      'Unknown simulator action restrat. Did you mean restart?',
    );
  });

  it('checks for Lima before it acts', async () => {
    vi.mocked(lima.requireLima).mockImplementationOnce(() => {
      throw new Error('Lima is not installed');
    });

    await expect(simulate('status')).rejects.toThrow('Lima is not installed');
    expect(status.printSimulatorStatus).not.toHaveBeenCalled();
  });

  it('presses keys on the chosen instance, in order', async () => {
    await simulate('key', 'Down', 'Enter', '--instance', 'ts');

    expect(simulator.requireRunningVm).toHaveBeenCalled();
    expect(simulator.pressKeys).toHaveBeenCalledWith(
      expect.anything(),
      expect.objectContaining({ name: 'ts' }),
      ['Down', 'Enter'],
    );
    expect(output.stdout()).toContain('Down Enter');
  });

  it('asks for the keys to press', async () => {
    await expect(simulate('key')).rejects.toThrow('simulator needs the keys to press');
  });

  it('takes the instance from RN_WEBOS_SIMULATOR_INSTANCE', async () => {
    vi.stubEnv('RN_WEBOS_SIMULATOR_INSTANCE', 'env-one');

    await simulate('pause');

    expect(simulator.sendLifecycle).toHaveBeenCalledWith(
      expect.anything(),
      expect.objectContaining({ name: 'env-one' }),
      'pause',
    );
  });

  it.each([
    ['resume', () => simulator.sendLifecycle],
    ['stop', () => simulator.stopSimulator],
    ['restart', () => simulator.startApp],
  ])('runs %s in the running VM', async (action, mock) => {
    await simulate(action);

    expect(mock()).toHaveBeenCalled();
  });

  it('opens a deep link', async () => {
    await simulate('open-url', 'app://a');

    expect(simulator.openUrl).toHaveBeenCalledWith(expect.anything(), expect.anything(), 'app://a');
  });

  it('restarts and relaunches the app with launch parameters', async () => {
    await simulate('restart', '--params', '{ "contentTarget": "app://a" }', '--screen', '1280x720');
    await simulate('relaunch', '--params', '{"contentTarget":"app://b"}');
    await simulate('relaunch');

    expect(simulator.startApp).toHaveBeenCalledWith(expect.anything(), expect.anything(), {
      launchParams: '{"contentTarget":"app://a"}',
      screen: '1280x720',
    });
    const relaunches = vi.mocked(simulator.sendControl).mock.calls.map((call) => call.slice(2));
    expect(relaunches).toEqual([
      ['relaunch', ['{"contentTarget":"app://b"}']],
      ['relaunch', ['{}']],
    ]);
  });

  it('refuses launch parameters that are not a JSON object', async () => {
    await expect(simulate('restart', '--params', '[1]')).rejects.toThrow('--params takes a JSON');
    await expect(simulate('restart', '--params', 'nope')).rejects.toThrow('--params takes a JSON');
  });

  it('types text into the focused field', async () => {
    await simulate('text', 'Blade', 'Runner');

    expect(simulator.sendControl).toHaveBeenCalledWith(
      expect.anything(),
      expect.anything(),
      'text',
      ['Blade Runner'],
    );
  });

  it("moves the remote's pointer, clicks and turns its wheel", async () => {
    await simulate('pointer', '960', '540');
    await simulate('click', '100', '200');
    await simulate('click');
    await simulate('wheel', '-3');

    const sent = vi.mocked(simulator.sendPointer).mock.calls.map((call) => call[2]);
    expect(sent).toEqual([
      [{ type: 'move', x: 960, y: 540 }],
      [{ type: 'move', x: 100, y: 200 }, { type: 'down' }, { type: 'up' }],
      [{ type: 'down' }, { type: 'up' }],
      [{ type: 'wheel', notches: -3 }],
    ]);
  });

  it('refuses a pointer position that is not two pixel numbers', async () => {
    await expect(simulate('pointer', 'left')).rejects.toThrow('The pointer takes x and y');
    await expect(simulate('wheel', 'down')).rejects.toThrow('wheel takes a number of notches');
  });

  it('asks open-url for its URL', async () => {
    await expect(simulate('open-url')).rejects.toThrow('simulator needs a URL');
  });

  it('saves a screenshot where --out says', async () => {
    await simulate('screenshot', '--out', '/tmp/shot.png');

    expect(simulator.saveScreenshot).toHaveBeenCalledWith(expect.anything(), '/tmp/shot.png');
  });

  it('saves a screenshot in the cache by default', async () => {
    vi.stubEnv('RN_WEBOS_CACHE_DIR', '/cache');

    await simulate('screenshot');

    expect(simulator.saveScreenshot).toHaveBeenCalledWith(
      expect.anything(),
      '/cache/simulator/default/screenshot.png',
    );
  });

  it('opens the live screen in the browser', async () => {
    await simulate('view');

    expect(viewer.openInBrowser).toHaveBeenCalledWith(expect.anything(), 'http://127.0.0.1:6080/');
    expect(output.stdout()).toContain('Arrows, Enter and Esc work as the remote');
  });

  it('prints the log with the number of lines asked for', async () => {
    await simulate('logs', '-n', '50', '-f');

    expect(simulator.printLog).toHaveBeenCalledWith(expect.anything(), { lines: 50, follow: true });
  });

  it('refuses a number of lines that is not one', async () => {
    await expect(simulate('logs', '-n', 'many')).rejects.toThrow(
      '--lines must be a whole number of lines, got many',
    );
    expect(simulator.printLog).not.toHaveBeenCalled();
  });

  it('prints the last crash', async () => {
    await simulate('crash');

    expect(output.stdout()).toContain('thread 1');
  });

  it('says which runtime builds it removed', async () => {
    await simulate('clean');

    expect(runtimes.cleanSimulatorRuntimes).toHaveBeenCalled();
    expect(output.stdout()).toContain('removed 0123456789ab');
  });

  it('deletes the VM', async () => {
    await simulate('delete');

    expect(lima.deleteInstance).toHaveBeenCalled();
  });

  it('hands run, build and status to their own modules', async () => {
    await simulate('run', '--no-logs');
    await simulate('build');
    await simulate('status');

    expect(simulatorRun.runInSimulator).toHaveBeenCalled();
    expect(simulatorRun.prepareSimulator).toHaveBeenCalled();
    expect(status.printSimulatorStatus).toHaveBeenCalled();
  });

  it("watches perf on the instance's own log port", async () => {
    const { watchPerf } = await import('../perf/watch-perf.ts');

    await simulate('perf', '--instance', 'ts', '--duration', '5');

    expect(watchPerf).toHaveBeenCalledWith(
      expect.anything(),
      expect.objectContaining({ durationMs: 5000, device: null }),
    );
  });
});
