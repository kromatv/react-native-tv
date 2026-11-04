import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { fakeApp } from '../app/app.fixtures.ts';
import { loadAppProject } from '../app/app-project.ts';
import { Reporter } from '../cli/reporter.ts';
import { capturedOutput, type CapturedOutput } from '../fakes.fixtures.ts';
import { listenForLogs, type LogServer } from '../logs/log-server.ts';
import { devServerBundleStep } from '../package/dev-server-bundle.ts';
import { bundleStep, tizenMetroStep } from '../pipeline/package-steps.ts';
import { simulatorInstance } from '../simulator/instance.ts';
import { ensureVm, hostAddress, startApp } from '../simulator/simulator.ts';
import { buildSimulatorRuntime } from '../simulator/simulator-runtime.ts';
import { stageApp } from '../simulator/stage-app.ts';
import { openInBrowser, startViewer } from '../simulator/viewer.ts';
import { prepareSimulator, runInSimulator } from './simulator-run.ts';

const ROOT = '/work/app';

vi.mock('../app/app-project.ts', () => ({ loadAppProject: vi.fn() }));
vi.mock('../logs/log-server.ts', () => ({ listenForLogs: vi.fn() }));
vi.mock('../pipeline/package-steps.ts', () => ({
  bundleStep: vi.fn(),
  bundleDir: vi.fn(() => '/b'),
  tizenMetroStep: vi.fn(),
}));
vi.mock('../package/dev-server-bundle.ts', () => ({ devServerBundleStep: vi.fn() }));
vi.mock('../runtime/icu-data.ts', () => ({
  ensureIcuData: vi.fn(async () => '/cache/icudt77l.dat'),
}));
vi.mock('../simulator/simulator.ts', () => ({
  ensureVm: vi.fn(async () => 'running'),
  hostAddress: vi.fn(async () => '192.168.5.2'),
  startApp: vi.fn(async () => 4242),
}));
vi.mock('../simulator/simulator-runtime.ts', () => ({ buildSimulatorRuntime: vi.fn() }));
vi.mock('../simulator/stage-app.ts', () => ({ stageApp: vi.fn() }));
vi.mock('../simulator/viewer.ts', async (importOriginal) => ({
  ...(await importOriginal<typeof import('../simulator/viewer.ts')>()),
  startViewer: vi.fn(async () => 'http://127.0.0.1:6080/'),
  openInBrowser: vi.fn(),
}));
vi.mock('../cli/log-level-option.ts', async (importOriginal) => ({
  ...(await importOriginal<typeof import('../cli/log-level-option.ts')>()),
  untilInterrupted: vi.fn(async () => undefined),
}));

const instance = simulatorInstance('ts');
const context = { cwd: ROOT };
let out: CapturedOutput;
let reporter: Reporter;
let server: LogServer;

beforeEach(() => {
  out = capturedOutput();
  reporter = new Reporter({ verbose: false, out, env: { NO_COLOR: '1' } });
  vi.mocked(loadAppProject).mockReturnValue(fakeApp(ROOT));
  vi.mocked(buildSimulatorRuntime).mockResolvedValue('0123456789ab');
  vi.mocked(bundleStep).mockResolvedValue({
    javascript: '/b/index.tizen.js',
    bytecode: '/b/index.tizen.hbc',
    assetsDir: '/b/assets',
  });
  server = { port: instance.logPort, capture: vi.fn(), close: vi.fn(async () => undefined) };
  vi.mocked(listenForLogs).mockResolvedValue(server);
});

afterEach(() => {
  vi.clearAllMocks();
});

describe('prepareSimulator', () => {
  it('makes the VM ready and builds the runtime for the app', async () => {
    const prepared = await prepareSimulator(reporter, {}, context);
    await prepareSimulator(reporter, { bundle: 'app.hbc' }, context);

    expect(tizenMetroStep).toHaveBeenCalledOnce();
    expect(prepared.runtime).toBe('0123456789ab');
    expect(ensureVm).toHaveBeenCalled();
    expect(out.text()).toContain('aarch64 Linux, 0123456789ab');
  });

  it('notes when it waits for another build of the same runtime', async () => {
    vi.mocked(buildSimulatorRuntime).mockImplementationOnce(async (_task, _app, onWait) => {
      onWait();
      return '';
    });

    await prepareSimulator(reporter, {}, context);

    expect(out.text()).toContain('Another simulator build is running');
  });
});

describe('runInSimulator', () => {
  it("bundles, stages and starts the app with its logs streaming to the instance's port", async () => {
    await runInSimulator(reporter, {}, context, instance);

    expect(listenForLogs).toHaveBeenCalledWith(instance.logPort, expect.anything(), {
      inUseHint: expect.stringContaining('--instance ts'),
    });
    expect(stageApp).toHaveBeenCalledWith(
      instance,
      expect.anything(),
      expect.objectContaining({ bytecode: '/b/index.tizen.hbc' }),
      { host: '192.168.5.2', port: instance.logPort },
      { key: '0123456789ab', icuData: '/cache/icudt77l.dat' },
    );
    expect(startApp).toHaveBeenCalled();
    expect(server.close).toHaveBeenCalled();
  });

  it('starts the bytecode --bundle names without bundling, and streams no logs with --no-logs', async () => {
    await runInSimulator(
      reporter,
      { bundle: import.meta.filename, 'no-logs': true },
      context,
      instance,
    );

    expect(bundleStep).not.toHaveBeenCalled();
    expect(listenForLogs).not.toHaveBeenCalled();
    expect(hostAddress).not.toHaveBeenCalled();
    expect(stageApp).toHaveBeenCalledWith(
      instance,
      expect.anything(),
      { bytecode: import.meta.filename },
      null,
      expect.anything(),
    );
  });

  it('refuses a --bundle that does not exist', async () => {
    await expect(
      runInSimulator(reporter, { bundle: 'missing.hbc', 'no-logs': true }, context, instance),
    ).rejects.toThrow(`The bytecode ${ROOT}/missing.hbc does not exist`);
    expect(stageApp).not.toHaveBeenCalled();
  });

  it('loads the bundle from the dev server with --dev-server', async () => {
    const devBundle = {
      javascript: '/b/index.tizen.js',
      bytecode: '/b/index.tizen.hbc',
      assetsDir: '/b/assets',
      bundleUrl: 'http://192.168.5.2:8081/.expo/.virtual-metro-entry.bundle?platform=tizen',
    };
    vi.mocked(devServerBundleStep).mockResolvedValue(devBundle);

    await runInSimulator(
      reporter,
      { 'dev-server': 'http://localhost:8081', 'no-logs': true },
      context,
      instance,
    );

    expect(bundleStep).not.toHaveBeenCalled();
    expect(devServerBundleStep).toHaveBeenCalledWith(
      reporter,
      expect.anything(),
      '/b',
      'http://localhost:8081',
      hostAddress,
    );
    expect(vi.mocked(stageApp).mock.calls[0]?.[2]).toBe(devBundle);
  });

  it('opens the live screen with --gui', async () => {
    await runInSimulator(reporter, { gui: true, 'no-logs': true }, context, instance);

    expect(startViewer).toHaveBeenCalledWith(expect.anything(), instance);
    expect(openInBrowser).toHaveBeenCalledWith(expect.anything(), 'http://127.0.0.1:6080/');
  });

  it('closes the log server when the start fails', async () => {
    vi.mocked(startApp).mockRejectedValueOnce(new Error('no window'));

    await expect(runInSimulator(reporter, {}, context, instance)).rejects.toThrow('no window');
    expect(server.close).toHaveBeenCalled();
  });
});
