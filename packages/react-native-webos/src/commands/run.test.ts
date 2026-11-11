import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { fakeApp } from '../app/app.fixtures.ts';
import { loadAppProject } from '../app/app-project.ts';
import { captureProcessOutput } from '../fakes.fixtures.ts';
import { listenForLogs, type LogServer } from '../logs/log-server.ts';
import { packageSteps } from '../pipeline/package-steps.ts';
import { useFakeTvs, useTemporaryState } from '../webos/connected-tv.fixtures.ts';
import { runCommand } from './run.ts';

const ROOT = '/work/app';
const INSTALL = '/usr/bin/luna-send-pub -i luna://com.webos.appInstallService/dev/install';
const LAUNCH = `/usr/bin/luna-send-pub -n 1 luna://com.webos.applicationManager//launch '{"id":"org.example.demo","subscribe":false,"params":{"displayAffinity":0}}'`;

vi.mock('../app/app-project.ts', () => ({ loadAppProject: vi.fn() }));
vi.mock('../pipeline/package-steps.ts', () => ({ packageSteps: vi.fn() }));
vi.mock('../logs/log-server.ts', () => ({ listenForLogs: vi.fn() }));
vi.mock('../cli/log-level-option.ts', async (importOriginal) => ({
  ...(await importOriginal<typeof import('../cli/log-level-option.ts')>()),
  untilInterrupted: vi.fn(async () => undefined),
}));

const context = { cwd: ROOT };
let output: ReturnType<typeof captureProcessOutput>;
let server: LogServer;

useTemporaryState();
const connect = useFakeTvs();

beforeEach(() => {
  output = captureProcessOutput();
  vi.mocked(loadAppProject).mockReturnValue(fakeApp(ROOT));
  vi.mocked(packageSteps).mockResolvedValue({
    ipk: import.meta.filename,
    appId: 'org.example.demo',
  });
  server = { port: 8099, capture: vi.fn(), close: vi.fn(async () => undefined) };
  vi.mocked(listenForLogs).mockResolvedValue(server);
});

afterEach(() => {
  vi.clearAllMocks();
});

describe('rn-webos run', () => {
  it('packages, installs and launches the app on one login, then prints its logs until Ctrl+C', async () => {
    const { tv } = await connect({ name: 'lounge' });

    await runCommand.run(['--log-host', '10.0.0.2'], context);

    expect(listenForLogs).toHaveBeenCalledWith(8099, expect.anything(), {
      peer: '127.0.0.1',
      app: 'org.example.demo',
    });
    expect(packageSteps).toHaveBeenCalledWith(expect.anything(), {
      app: expect.objectContaining({ root: ROOT }),
      log: { host: '10.0.0.2', port: 8099 },
      devServer: undefined,
    });
    expect(tv.commands.filter((command) => command.startsWith(INSTALL))).toHaveLength(1);
    expect(tv.commands.at(-1)).toBe(LAUNCH);
    expect(tv.connections).toBe(2);
    expect(output.stdout()).toMatch(/TV\s+lounge, OLED65C25LB, webOS 26 \(11\.2\.0\)/);
    expect(output.stdout()).toContain('Logs from org.example.demo. Press Ctrl+C to stop.');
    expect(server.close).toHaveBeenCalled();
  });

  it('listens for nothing with --no-logs or --release', async () => {
    const { tv } = await connect({ name: 'lounge' });

    await runCommand.run(['--no-logs', '--log-host', '10.0.0.2', '--device', 'lounge'], context);
    await runCommand.run(['--release'], context);

    expect(listenForLogs).not.toHaveBeenCalled();
    expect(tv.commands.filter((command) => command === LAUNCH)).toHaveLength(2);
    expect(vi.mocked(packageSteps).mock.calls[1]?.[1]).toMatchObject({ log: null });
  });

  it('closes the log server when the build fails, and installs nothing', async () => {
    const { tv } = await connect();
    vi.mocked(packageSteps).mockRejectedValueOnce(new Error('the compiler failed'));

    await expect(runCommand.run(['--log-host', '10.0.0.2'], context)).rejects.toThrow(
      'the compiler failed',
    );
    expect(server.close).toHaveBeenCalled();
    expect(tv.commands.some((command) => command.startsWith(INSTALL))).toBe(false);
  });

  it('builds nothing for a TV rn-webos is not connected to', async () => {
    await expect(runCommand.run(['--device', '192.168.1.77'], context)).rejects.toThrow(
      'rn-webos is not connected to the TV at 192.168.1.77',
    );
    expect(packageSteps).not.toHaveBeenCalled();
  });
});
