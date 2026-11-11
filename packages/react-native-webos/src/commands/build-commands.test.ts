import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { fakeApp } from '../app/app.fixtures.ts';
import { loadAppProject } from '../app/app-project.ts';
import { captureProcessOutput } from '../fakes.fixtures.ts';
import { bundleStep, packageSteps, appSetupSteps } from '../pipeline/package-steps.ts';
import { runtimeSteps } from '../pipeline/runtime-steps.ts';
import { ensureWebOSSdk } from '../sysroot/webos-sdk.ts';
import { buildRuntimeCommand } from './build-runtime.ts';
import { bundleCommand } from './bundle.ts';
import { packageCommand } from './package.ts';
import { setupCommand } from './setup.ts';

const ROOT = '/work/app';
const SDK = { path: '/cache/sdk/2026.08-webos', version: '2026.08-webos', reused: true };

vi.mock('../app/app-project.ts', () => ({ loadAppProject: vi.fn() }));
vi.mock('../pipeline/package-steps.ts', () => ({
  bundleStep: vi.fn(),
  packageSteps: vi.fn(),
  appSetupSteps: vi.fn(),
}));
vi.mock('../pipeline/runtime-steps.ts', () => ({ runtimeSteps: vi.fn() }));
vi.mock('../sysroot/webos-sdk.ts', () => ({ ensureWebOSSdk: vi.fn() }));

const context = { cwd: ROOT };
let output: ReturnType<typeof captureProcessOutput>;

beforeEach(() => {
  output = captureProcessOutput();
  vi.mocked(loadAppProject).mockReturnValue(fakeApp(ROOT));
  vi.mocked(packageSteps).mockResolvedValue({
    ipk: import.meta.filename,
    appId: 'org.example.demo',
  });
  vi.mocked(bundleStep).mockResolvedValue({
    javascript: `${ROOT}/webos/build/bundle/index.webos.js`,
    bytecode: import.meta.filename,
    assetsDir: `${ROOT}/webos/build/bundle/assets`,
  });
  vi.mocked(runtimeSteps).mockResolvedValue('/cache/runtime/build/host/react-native-webos');
  vi.mocked(ensureWebOSSdk).mockResolvedValue(SDK);
});

afterEach(() => {
  vi.clearAllMocks();
  vi.restoreAllMocks();
});

describe('rn-webos bundle', () => {
  it("bundles into the app's webos/ unless --out says otherwise", async () => {
    await bundleCommand.run([], context);
    await bundleCommand.run(['--out', 'dist'], context);

    expect(appSetupSteps).toHaveBeenCalledTimes(2);
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

describe('rn-webos package', () => {
  it('builds a release package by default', async () => {
    await packageCommand.run(['--device', '192.168.1.50'], context);

    expect(packageSteps).toHaveBeenCalledWith(expect.anything(), {
      app: fakeApp(ROOT),
      log: null,
      devServer: undefined,
    });
    expect(output.stdout()).toContain('release');
    expect(output.stdout()).toContain('rn-webos run --device 192.168.1.50');
  });

  it('builds a development package streaming its logs with --dev', async () => {
    await packageCommand.run(['--dev', '--log-host', '10.0.0.2', '--log-port', '9000'], context);

    expect(packageSteps).toHaveBeenCalledWith(
      expect.anything(),
      expect.objectContaining({ log: { host: '10.0.0.2', port: 9000 } }),
    );
    expect(output.stdout()).toContain('development, logs to 10.0.0.2:9000');
    expect(output.stdout()).toContain('rn-webos run --device <tv ip>');
  });

  it('hands the dev server of a development build to the pipeline', async () => {
    await packageCommand.run(
      ['--dev', '--log-host', '10.0.0.2', '--dev-server', 'http://localhost:8081'],
      context,
    );

    expect(packageSteps).toHaveBeenCalledWith(
      expect.anything(),
      expect.objectContaining({ devServer: 'http://localhost:8081' }),
    );
  });

  it('refuses the options of a development build without --dev', async () => {
    for (const flag of [
      ['--log-port', '9000'],
      ['--dev-server', 'http://localhost:8081'],
    ]) {
      await expect(packageCommand.run(flag, context)).rejects.toThrow(
        '--log-host, --log-port and --dev-server set up a development build: add --dev',
      );
    }
  });

  it("refuses a --device that is not the TV's IP address", async () => {
    await expect(packageCommand.run(['--device', 'living-room'], context)).rejects.toThrow(
      "--device must be the TV's IP address, got living-room",
    );
  });

  it('takes no option for a CPU, a platform floor or a signing identity: every build is the same', async () => {
    for (const flag of ['--arch', '--platform', '--profile', '--author', '--distributor']) {
      await expect(packageCommand.run([flag, 'x'], context)).rejects.toThrow();
    }
  });
});

describe('rn-webos build-runtime', () => {
  it('builds the development runtime, the one run packages', async () => {
    await buildRuntimeCommand.run([], context);

    expect(runtimeSteps).toHaveBeenCalledWith(expect.anything(), fakeApp(ROOT), 'development');
    expect(output.stdout()).toContain('/cache/runtime/build/host/react-native-webos');
  });

  it('builds the release runtime when asked', async () => {
    await buildRuntimeCommand.run(['--release'], context);

    expect(runtimeSteps).toHaveBeenCalledWith(expect.anything(), expect.anything(), 'release');
  });
});

describe('rn-webos setup', () => {
  it('fetches the SDK and says where it is', async () => {
    await setupCommand.run([], context);

    expect(ensureWebOSSdk).toHaveBeenCalledOnce();
    expect(output.stdout()).toContain('2026.08-webos, already set up');
    expect(output.stdout()).toContain('/cache/sdk/2026.08-webos');
  });
});
