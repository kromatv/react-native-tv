import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { scanForTvs } from '../discovery/scan-tvs.ts';
import { captureProcessOutput } from '../fakes.fixtures.ts';
import { useFakeTvs, useTemporaryState } from '../webos/connected-tv.fixtures.ts';
import { devicesCommand } from './devices.ts';

vi.mock('../discovery/scan-tvs.ts', () => ({ scanForTvs: vi.fn() }));

let output: ReturnType<typeof captureProcessOutput>;

const state = useTemporaryState();
const connect = useFakeTvs();

beforeEach(() => {
  output = captureProcessOutput();
});

afterEach(() => {
  vi.restoreAllMocks();
});

describe('rn-webos devices', () => {
  it('says how to connect a TV when none is known', async () => {
    await devicesCommand.run(['--no-scan'], { cwd: state() });

    expect(output.stdout()).toContain('No LG TV found');
    expect(output.stdout()).toContain('rn-webos connect <tv ip>');
    expect(scanForTvs).not.toHaveBeenCalled();
  });

  it('logs in to a connected TV the scan did not find, and lists what it reports', async () => {
    const { tv } = await connect({ name: 'lounge' });

    await devicesCommand.run(['--no-scan'], { cwd: state() });

    expect(output.stdout()).toContain('127.0.0.1  OLED65C25LB');
    expect(output.stdout()).toContain('webOS 26 (11.2.0)');
    expect(output.stdout()).toMatch(/Developer Mode\s+on\n/);
    expect(output.stdout()).toContain('connected as lounge');
    expect(output.stdout()).toContain('1 of 1 connected');
    expect(tv.connections).toBe(1);
  });
});
