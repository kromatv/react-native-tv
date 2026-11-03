import { mkdtempSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { probeTv, type SdbState } from '../discovery/probe-tv.ts';
import { scanForTvs, type FoundTv } from '../discovery/scan-tvs.ts';
import { readKnownDevices, rememberDevices } from '../tizen/known-devices.ts';
import { devicesCommand } from './devices.ts';

vi.mock('../discovery/scan-tvs.ts', () => ({ scanForTvs: vi.fn() }));
vi.mock('../discovery/probe-tv.ts', () => ({ probeTv: vi.fn() }));

const READY: SdbState = {
  state: 'ready',
  capabilities: { cpuArch: 'armv7', platformVersion: '9.0', profile: 'tv', toolPath: null },
  model: 'QE55Q80',
  duid: 'DUID-ONE',
};

interface Row {
  readonly tv: FoundTv;
  readonly hostPc: boolean | null;
  readonly sdb: SdbState;
}

const found = (host: string, developerMode: boolean | null, developerIp: string | null) => ({
  host,
  name: 'Living room',
  model: 'QE55Q80',
  developerMode,
  developerIp,
  sdbPort: 26101,
});

const ROWS: readonly Row[] = [
  { tv: found('10.0.0.1', true, '10.0.0.9'), hostPc: true, sdb: READY },
  {
    tv: found('10.0.0.2', true, '10.0.0.8'),
    hostPc: false,
    sdb: { state: 'ready', capabilities: null, model: null, duid: null },
  },
  { tv: found('10.0.0.3', true, null), hostPc: null, sdb: { state: 'refused', reason: '' } },
  { tv: found('10.0.0.4', false, null), hostPc: null, sdb: { state: 'closed', reason: '' } },
  { tv: found('10.0.0.5', null, null), hostPc: null, sdb: { state: 'silent', reason: '' } },
];

let state = '';
let printed = '';

beforeEach(() => {
  state = mkdtempSync(join(tmpdir(), 'rn-tizen-devices-report-'));
  vi.stubEnv('XDG_STATE_HOME', state);
  printed = '';
  vi.spyOn(process.stdout, 'write').mockImplementation((chunk) => {
    printed += String(chunk);
    return true;
  });
  vi.mocked(probeTv).mockImplementation((tv) => {
    const row = ROWS.find((candidate) => candidate.tv.host === tv.host);
    return Promise.resolve({
      ...tv,
      hostPcIsThisComputer: row?.hostPc ?? null,
      sdb: row?.sdb ?? { state: 'closed', reason: '' },
    });
  });
});

afterEach(() => {
  vi.restoreAllMocks();
  vi.unstubAllEnvs();
  rmSync(state, { recursive: true, force: true });
});

describe('rn-tizen devices report', () => {
  it('says what each TV found on the network answers, and remembers the ready ones', async () => {
    await rememberDevices([
      { host: '10.0.0.1', port: 26101 },
      { host: '10.0.0.6', port: 26101 },
    ]);
    vi.mocked(scanForTvs).mockImplementationOnce((options) => {
      options?.onProgress?.(1, 254);
      return Promise.resolve(ROWS.map((row) => row.tv));
    });

    await devicesCommand.run([], { cwd: state });

    for (const text of [
      '5 Samsung TVs',
      'on, host PC 10.0.0.9 (this computer)',
      'ready: armv7, Tizen 9.0',
      'DUID-ONE',
      'on, host PC 10.0.0.8 (not this computer)',
      'on, no host PC set',
      'busy: another computer',
      'off: on the TV, open Apps',
      'closed: port 26101',
      'unknown: the TV did not answer',
      'never answered',
      '2 of 6 ready for sdb',
    ]) {
      expect(printed).toContain(text);
    }
    expect((await readKnownDevices()).map((device) => device.host)).toEqual([
      '10.0.0.1',
      '10.0.0.2',
      '10.0.0.6',
    ]);
  });

  it('counts one TV as one', async () => {
    vi.mocked(scanForTvs).mockResolvedValueOnce([found('10.0.0.1', true, '10.0.0.9')]);

    await devicesCommand.run([], { cwd: state });

    expect(printed).toMatch(/1 Samsung TV(?!s)/);
  });
});
