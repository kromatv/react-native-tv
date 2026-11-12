import { afterEach, describe, expect, it, vi } from 'vitest';
import { Reporter } from '../cli/reporter.ts';
import { capturedOutput } from '../fakes.fixtures.ts';
import { simulatorInstance } from '../simulator/instance.ts';
import { instanceState } from '../simulator/lima.ts';
import { readSimulatorStatus } from '../simulator/simulator-status.ts';
import { printSimulatorStatus } from './simulator-status.ts';

vi.mock('../simulator/lima.ts', () => ({ instanceState: vi.fn() }));
vi.mock('../simulator/simulator-status.ts', async (importOriginal) => ({
  ...(await importOriginal<typeof import('../simulator/simulator-status.ts')>()),
  readSimulatorStatus: vi.fn(),
}));

function instanceStatus(name: string, running: boolean) {
  return {
    name,
    staged: running,
    app: running ? 4301 : null,
    lastApp: running ? 4301 : null,
    crashed: false,
    screen: running ? 4290 : null,
    viewer: running ? 4291 : null,
    runtime: running ? '0123456789ab' : '',
    runtimeBuilt: running,
  };
}

async function printed(current = 'ts'): Promise<string> {
  const out = capturedOutput();
  const reporter = new Reporter({ verbose: false, out, env: { NO_COLOR: '1' } });
  await printSimulatorStatus(reporter, simulatorInstance(current));
  return out.text();
}

afterEach(() => {
  vi.clearAllMocks();
});

describe('printSimulatorStatus', () => {
  it('says how to create a VM that does not exist', async () => {
    vi.mocked(instanceState).mockResolvedValue(null);

    expect(await printed()).toContain('not created: rn-tizen simulator run');
    expect(readSimulatorStatus).not.toHaveBeenCalled();
  });

  it('stops at the state of a VM that is not running', async () => {
    vi.mocked(instanceState).mockResolvedValue({ status: 'Stopped' });

    expect(await printed()).toMatch(/VM\s+stopped\n$/);
  });

  it('prints the runtimes, this instance first, then every other one', async () => {
    vi.mocked(instanceState).mockResolvedValue({ status: 'Running' });
    vi.mocked(readSimulatorStatus).mockResolvedValue({
      runtimes: [{ key: '0123456789ab', built: true }],
      instances: [instanceStatus('other', true), instanceStatus('ts', false)],
    });

    const text = await printed();

    expect(text).toContain('0123456789ab');
    expect(text.indexOf('ts  (this one)')).toBeLessThan(text.indexOf('other'));
    expect(text).toContain('off, port');
    expect(text).toContain(':');
  });

  it('says how to build a runtime when there is none', async () => {
    vi.mocked(instanceState).mockResolvedValue({ status: 'Running' });
    vi.mocked(readSimulatorStatus).mockResolvedValue({ runtimes: [], instances: [] });

    expect(await printed()).toContain('none built: rn-tizen simulator build');
  });
});
