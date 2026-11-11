import { describe, expect, it, vi } from 'vitest';
import type { FoundTv } from '../discovery/scan-tvs.ts';
import { lastOrOnlyTv, webosTargets } from './webos-targets.ts';

const tv = (host: string, developerMode = true): FoundTv => ({
  host,
  name: '[LG] webOS TV OLED65C25LB',
  model: 'OLED65C25LB',
  id: null,
  webosVersion: '11.2.0',
  developerMode,
  keyServer: false,
  sshPort: 9922,
});

const none = { knownDevices: async () => [], scan: async () => [] };

describe('lastOrOnlyTv', () => {
  it('takes the TV used last without scanning', async () => {
    const scan = vi.fn(async () => []);
    const knownDevices = async () => [{ name: 'lounge', host: '10.0.0.7', port: 9922 }];

    expect(await lastOrOnlyTv({ knownDevices, scan })).toBe('10.0.0.7');
    expect(scan).not.toHaveBeenCalled();
  });

  it('else scans, and names the TVs in Developer Mode to connect first', async () => {
    const searching = vi.fn();

    const opening = lastOrOnlyTv({
      ...none,
      scan: async () => [tv('10.0.0.3', false), tv('10.0.0.4')],
      searching,
    });

    await expect(opening).rejects.toThrow(
      'rn-webos is not connected to a TV yet. In Developer Mode on this network: 10.0.0.4. Turn Key Server on there and run: rn-webos connect <tv ip>',
    );
    expect(searching).toHaveBeenCalledOnce();
  });

  it('says what to do when the network has no TV in Developer Mode', async () => {
    await expect(lastOrOnlyTv(none)).rejects.toThrow('No TV in Developer Mode answered');
  });
});

describe('webosTargets', () => {
  it('lists the TVs connected before, the last one used first, then the simulator', async () => {
    const targets = await webosTargets({
      knownDevices: async () => [
        { name: 'lounge', host: '10.0.0.7', port: 9922 },
        { name: 'OLED65C25LB', host: '10.0.0.8', port: 9923 },
      ],
    });

    expect(targets).toEqual([
      { target: '10.0.0.7', label: '10.0.0.7  lounge  (used last)' },
      { target: '10.0.0.8:9923', label: '10.0.0.8  OLED65C25LB' },
      { target: 'simulator', label: 'webOS simulator' },
    ]);
  });
});
