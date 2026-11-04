import { describe, expect, it, vi } from 'vitest';
import type { FoundTv } from '../discovery/scan-tvs.ts';
import { lastOrOnlyTv, tizenTargets } from './tizen-targets.ts';

const tv = (host: string, developerMode: boolean | null = true): FoundTv => ({
  host,
  name: 'Living room',
  model: 'QE55',
  developerMode,
  developerIp: null,
  sdbPort: 26101,
});

const none = { knownDevices: async () => [], scan: async () => [] };

describe('lastOrOnlyTv', () => {
  it('takes the TV used last without scanning', async () => {
    const scan = vi.fn(async () => []);

    expect(
      await lastOrOnlyTv({ knownDevices: async () => [{ host: '10.0.0.7', port: 26101 }], scan }),
    ).toBe('10.0.0.7:26101');
    expect(scan).not.toHaveBeenCalled();
  });

  it('else scans, and takes the only TV not known to be out of Developer mode', async () => {
    const searching = vi.fn();

    const target = await lastOrOnlyTv({
      ...none,
      scan: async () => [tv('10.0.0.3', false), tv('10.0.0.4', null)],
      searching,
    });

    expect(target).toBe('10.0.0.4:26101');
    expect(searching).toHaveBeenCalledOnce();
  });

  it('says what to do when the network has no TV, or several', async () => {
    await expect(lastOrOnlyTv(none)).rejects.toThrow('No TV in Developer mode answered');
    await expect(
      lastOrOnlyTv({ ...none, scan: async () => [tv('10.0.0.3'), tv('10.0.0.4')] }),
    ).rejects.toThrow('2 TVs answered (10.0.0.3, 10.0.0.4): press shift+t to pick one.');
  });
});

describe('tizenTargets', () => {
  it('lists the TVs used before, then those found, each once, then the simulator', async () => {
    const targets = await tizenTargets({
      knownDevices: async () => [
        { host: '10.0.0.7', port: 26101 },
        { host: '10.0.0.8', port: 26101 },
      ],
      scan: async () => [tv('10.0.0.7'), tv('10.0.0.9'), tv('10.0.0.5', false)],
    });

    expect(targets).toEqual([
      { target: '10.0.0.7:26101', label: '10.0.0.7  Living room QE55  (used last)' },
      { target: '10.0.0.8:26101', label: '10.0.0.8' },
      { target: '10.0.0.9:26101', label: '10.0.0.9  Living room QE55' },
      { target: 'simulator', label: 'Tizen simulator' },
    ]);
  });
});
