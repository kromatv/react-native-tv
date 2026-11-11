import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { probeTv, type ConnectionState } from '../discovery/probe-tv.ts';
import { scanForTvs, type FoundTv } from '../discovery/scan-tvs.ts';
import { captureProcessOutput } from '../fakes.fixtures.ts';
import { useTemporaryState } from '../webos/connected-tv.fixtures.ts';
import { rememberDevice } from '../webos/known-devices.ts';
import { devicesCommand } from './devices.ts';

vi.mock('../discovery/scan-tvs.ts', () => ({ scanForTvs: vi.fn() }));
vi.mock('../discovery/probe-tv.ts', () => ({ probeTv: vi.fn() }));

const INFO = { modelName: 'OLED65C25LB', sdkVersion: '11.2.0', firmwareVersion: '43.21.74' };

const found = (host: string, developerMode: boolean, keyServer: boolean): FoundTv => ({
  host,
  name: '[LG] webOS TV OLED65C25LB',
  model: 'OLED65C25LB',
  id: null,
  webosVersion: '11.2.0',
  developerMode,
  keyServer,
  sshPort: 9922,
});

const ROWS: readonly (readonly [FoundTv, ConnectionState])[] = [
  [found('10.0.0.1', true, false), { state: 'ready', name: 'lounge', info: INFO }],
  [found('10.0.0.2', true, true), { state: 'unknown' }],
  [found('10.0.0.3', false, false), { state: 'unknown' }],
  [
    found('10.0.0.4', true, false),
    { state: 'failed', name: 'office', reason: '10.0.0.4 refused the key rn-webos has for it' },
  ],
];

let output: ReturnType<typeof captureProcessOutput>;
const state = useTemporaryState();

beforeEach(() => {
  output = captureProcessOutput();
  vi.mocked(probeTv).mockImplementation((tv) => {
    const row = ROWS.find(([candidate]) => candidate.host === tv.host);
    return Promise.resolve({ ...tv, connection: row?.[1] ?? { state: 'unknown' } });
  });
});

afterEach(() => {
  vi.restoreAllMocks();
});

describe('rn-webos devices report', () => {
  it('says what each TV found on the network answers, and what rn-webos needs of it', async () => {
    await rememberDevice({ name: 'gone', host: '10.0.0.6', port: 9922 });
    vi.mocked(scanForTvs).mockImplementationOnce((options) => {
      options?.onProgress?.(1, 254);
      return Promise.resolve(ROWS.map(([tv]) => tv));
    });

    await devicesCommand.run([], { cwd: state() });

    for (const text of [
      '4 LG TVs',
      '10.0.0.1  [LG] webOS TV OLED65C25LB  OLED65C25LB',
      'webOS 26 (11.2.0)',
      'connected as lounge',
      'off, and not needed while rn-webos is connected',
      'not connected: rn-webos connect 10.0.0.2',
      "off: install the Developer Mode app from LG's store",
      'off: turn Key Server on in the Developer Mode app',
      'connected as office, but 10.0.0.4 refused the key rn-webos has for it',
      '1 of 5 connected',
    ]) {
      expect(output.stdout()).toContain(text);
    }
    expect(vi.mocked(probeTv).mock.calls.map(([tv]) => tv.host)).toContain('10.0.0.6');
  });

  it('counts one TV as one', async () => {
    vi.mocked(scanForTvs).mockResolvedValueOnce([found('10.0.0.1', true, false)]);

    await devicesCommand.run([], { cwd: state() });

    expect(output.stdout()).toMatch(/1 LG TV(?!s)/);
  });
});
