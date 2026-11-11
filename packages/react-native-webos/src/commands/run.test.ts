import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { fakeApp } from '../app/app.fixtures.ts';
import { loadAppProject } from '../app/app-project.ts';
import { captureProcessOutput } from '../fakes.fixtures.ts';
import { listenForLogs, type LogServer } from '../logs/log-server.ts';
import { bundleStep, packageSteps } from '../pipeline/package-steps.ts';
import { testSigner } from '../signing/signing.fixtures.ts';
import { resolveDevice } from '../webos/devices.ts';
import { installOn, launchOn, onDevice } from '../webos/install-app.ts';
import { runCommand } from './run.ts';

const ROOT = '/work/app';
const SDK = { root: '/webos-studio', packageManager: '/pm' };
const [DISTRIBUTOR] = testSigner('Distributor').chain;
const TV = {
  address: { host: '192.168.1.50', port: 26101 },
  serial: '192.168.1.50:26101',
  name: 'Living room',
  capabilities: { cpuArch: 'armv7', platformVersion: '9.0', profile: 'tv', toolPath: null },
};

vi.mock('../app/app-project.ts', () => ({ loadAppProject: vi.fn() }));
vi.mock('../webos/webos-sdk.ts', () => ({ locateWebOSSdk: () => SDK }));
vi.mock('../webos/devices.ts', () => ({ resolveDevice: vi.fn() }));
const CONNECTION = { address: { host: '192.168.1.50', port: 26101 } };
vi.mock('../webos/install-app.ts', () => ({
  onDevice: vi.fn(async (_device: unknown, work: (tv: unknown) => Promise<unknown>) =>
    work(CONNECTION),
  ),
  installOn: vi.fn(),
  launchOn: vi.fn(),
}));
vi.mock('../pipeline/package-steps.ts', () => ({
  bundleStep: vi.fn(),
  packageSteps: vi.fn(),
  appSetupSteps: vi.fn(),
}));
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
    javascript: `${ROOT}/webos/build/bundle/index.webos.js`,
    bytecode: import.meta.filename,
    assetsDir: `${ROOT}/webos/build/bundle/assets`,
  });
  server = { port: 8099, capture: vi.fn(), close: vi.fn(async () => undefined) };
  vi.mocked(listenForLogs).mockResolvedValue(server);
});

afterEach(() => {
  vi.clearAllMocks();
});

describe('rn-webos run', () => {
  it('packages, installs and launches the app, then prints its logs until Ctrl+C', async () => {
    await runCommand.run(['--log-host', '10.0.0.2'], context);

    expect(listenForLogs).toHaveBeenCalledWith(8099, expect.anything(), {
      peer: '192.168.1.50',
      app: 'org.example.demo',
    });
    expect(onDevice).toHaveBeenCalledOnce();
    expect(installOn).toHaveBeenCalledWith(CONNECTION, expect.anything(), TV, {
      file: import.meta.filename,
      installId: 'org.example.demo',
      duids: [],
    });
    expect(launchOn).toHaveBeenCalledWith(CONNECTION, TV, 'org.example.demo');
    expect(server.close).toHaveBeenCalled();
  });

  it('listens for nothing with --no-logs or --release', async () => {
    await runCommand.run(['--no-logs', '--log-host', '10.0.0.2'], context);
    await runCommand.run(['--release'], context);

    expect(listenForLogs).not.toHaveBeenCalled();
    expect(launchOn).toHaveBeenCalledTimes(2);
  });

  it('closes the log server when the build fails', async () => {
    vi.mocked(packageSteps).mockRejectedValueOnce(new Error('dotnet failed'));

    await expect(runCommand.run(['--log-host', '10.0.0.2'], context)).rejects.toThrow(
      'dotnet failed',
    );
    expect(server.close).toHaveBeenCalled();
  });
});
