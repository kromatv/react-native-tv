import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { fakeApp } from '../app/app.fixtures.ts';
import { loadAppProject } from '../app/app-project.ts';
import { captureProcessOutput } from '../fakes.fixtures.ts';
import { listenForLogs, type LogServer } from '../logs/log-server.ts';
import { bundleStep, packageSteps } from '../pipeline/package-steps.ts';
import { runtimeSteps } from '../pipeline/runtime-steps.ts';
import { testSigner } from '../signing/signing.fixtures.ts';
import { setupTarget } from '../sysroot/setup-target.ts';
import { resolveDevice } from '../tizen/devices.ts';
import { installApp, launchApp } from '../tizen/install-app.ts';
import { buildRuntimeCommand } from './build-runtime.ts';
import { bundleCommand } from './bundle.ts';
import { packageCommand } from './package.ts';
import { runCommand } from './run.ts';
import { setupCommand } from './setup.ts';

const ROOT = '/work/app';
const SDK = { root: '/tizen-studio', packageManager: '/pm' };
const [DISTRIBUTOR] = testSigner('Distributor').chain;
const TV = {
  address: { host: '192.168.1.50', port: 26101 },
  serial: '192.168.1.50:26101',
  name: 'Living room',
  capabilities: { cpuArch: 'armv7', platformVersion: '9.0', profile: 'tv', toolPath: null },
};

vi.mock('../app/app-project.ts', () => ({ loadAppProject: vi.fn() }));
vi.mock('../tizen/tizen-sdk.ts', () => ({ locateTizenSdk: () => SDK }));
vi.mock('../tizen/devices.ts', () => ({ resolveDevice: vi.fn() }));
vi.mock('../tizen/install-app.ts', () => ({ installApp: vi.fn(), launchApp: vi.fn() }));
vi.mock('../pipeline/package-steps.ts', () => ({ bundleStep: vi.fn(), packageSteps: vi.fn() }));
vi.mock('../pipeline/runtime-steps.ts', () => ({ runtimeSteps: vi.fn() }));
vi.mock('../sysroot/setup-target.ts', () => ({ setupTarget: vi.fn() }));
vi.mock('../logs/log-server.ts', () => ({ listenForLogs: vi.fn() }));
vi.mock('../cli/log-level-option.ts', async (importOriginal) => ({
  ...(await importOriginal<typeof import('../cli/log-level-option.ts')>()),
  untilInterrupted: vi.fn(async () => undefined),
}));

const context = { cwd: ROOT };
let output: ReturnType<typeof captureProcessOutput>;
let server: LogServer;

beforeEach(async () => {
  output = captureProcessOutput();
  vi.mocked(loadAppProject).mockReturnValue(fakeApp(ROOT));
  vi.mocked(resolveDevice).mockResolvedValue(TV);
  if (DISTRIBUTOR === undefined) throw new Error('the test signer has no certificate');
  vi.mocked(packageSteps).mockResolvedValue({
    tpk: import.meta.filename,
    distributor: DISTRIBUTOR,
  });
  vi.mocked(bundleStep).mockResolvedValue({
    javascript: `${ROOT}/tizen/build/bundle/index.tizen.js`,
    bytecode: import.meta.filename,
    assetsDir: `${ROOT}/tizen/build/bundle/assets`,
  });
  vi.mocked(runtimeSteps).mockResolvedValue('/cache/runtime/libreact-native-tizen.so');
  vi.mocked(setupTarget).mockResolvedValue({
    path: '/cache/sysroot',
    reused: true,
    patchedLibraries: 0,
  });
  server = { port: 8099, capture: vi.fn(), close: vi.fn(async () => undefined) };
  vi.mocked(listenForLogs).mockResolvedValue(server);
});

afterEach(() => {
  vi.clearAllMocks();
});

describe('rn-tizen bundle', () => {
  it("bundles into the app's tizen/ unless --out says otherwise", async () => {
    await bundleCommand.run([], context);
    await bundleCommand.run(['--out', 'dist'], context);

    expect(vi.mocked(bundleStep).mock.calls.map((call) => call[2])).toEqual([
      undefined,
      `${ROOT}/dist`,
    ]);
    expect(output.stdout()).toContain('Bytecode');
  });

  it('bundles the entry --entry names, resolved against the working directory', async () => {
    await bundleCommand.run(['--entry', 'src/other.ts', '--project', '../app'], context);

    expect(loadAppProject).toHaveBeenCalledWith('/work/app', `${ROOT}/src/other.ts`);
  });
});

describe('rn-tizen package', () => {
  it('builds for the store by default, for the TV it was given', async () => {
    await packageCommand.run(['--device', '192.168.1.50'], context);

    expect(resolveDevice).toHaveBeenCalledWith('192.168.1.50');
    expect(packageSteps).toHaveBeenCalledWith(
      expect.anything(),
      expect.objectContaining({ log: null, target: { arch: 'armv7l', platformVersion: '9.0' } }),
    );
    expect(output.stdout()).toContain('rn-tizen run --device 192.168.1.50:26101');
  });

  it('builds a development package streaming its logs with --dev', async () => {
    await packageCommand.run(['--dev', '--log-host', '10.0.0.2', '--log-port', '9000'], context);

    expect(resolveDevice).not.toHaveBeenCalled();
    expect(packageSteps).toHaveBeenCalledWith(
      expect.anything(),
      expect.objectContaining({ log: { host: '10.0.0.2', port: 9000 } }),
    );
    expect(output.stdout()).toContain('rn-tizen run --device <tv ip>');
  });

  it('refuses log options without --dev', async () => {
    await expect(packageCommand.run(['--log-port', '9000'], context)).rejects.toThrow(
      '--log-host and --log-port set up a development build: add --dev',
    );
  });
});

describe('rn-tizen run', () => {
  it('packages, installs and launches the app, then prints its logs until Ctrl+C', async () => {
    await runCommand.run(['--log-host', '10.0.0.2'], context);

    expect(listenForLogs).toHaveBeenCalledWith(8099, expect.anything(), { peer: '192.168.1.50' });
    expect(installApp).toHaveBeenCalledWith(expect.anything(), TV, {
      file: import.meta.filename,
      installId: 'org.example.demo',
      duids: [],
    });
    expect(launchApp).toHaveBeenCalledWith(TV, 'org.example.demo');
    expect(server.close).toHaveBeenCalled();
  });

  it('listens for nothing with --no-logs or --release', async () => {
    await runCommand.run(['--no-logs', '--log-host', '10.0.0.2'], context);
    await runCommand.run(['--release'], context);

    expect(listenForLogs).not.toHaveBeenCalled();
    expect(launchApp).toHaveBeenCalledTimes(2);
  });

  it('closes the log server when the build fails', async () => {
    vi.mocked(packageSteps).mockRejectedValueOnce(new Error('dotnet failed'));

    await expect(runCommand.run(['--log-host', '10.0.0.2'], context)).rejects.toThrow(
      'dotnet failed',
    );
    expect(server.close).toHaveBeenCalled();
  });
});

describe('rn-tizen build-runtime', () => {
  it("builds for the app's platform unless told otherwise", async () => {
    vi.mocked(loadAppProject).mockReturnValue(fakeApp(ROOT, { platform: '8.0' }));

    await buildRuntimeCommand.run(['--arch', 'aarch64'], context);

    expect(runtimeSteps).toHaveBeenCalledWith(expect.anything(), SDK, expect.anything(), {
      arch: 'aarch64',
      platformVersion: '8.0',
    });
    expect(output.stdout()).toContain('/cache/runtime/libreact-native-tizen.so');
  });
});

describe('rn-tizen setup', () => {
  it('prepares the target the flags name', async () => {
    await setupCommand.run(['--arch', 'aarch64', '--platform', '9.0'], context);

    expect(setupTarget).toHaveBeenCalledWith(expect.anything(), SDK, {
      arch: 'aarch64',
      platformVersion: '9.0',
    });
    expect(output.stdout()).toContain('already set up');
  });
});
