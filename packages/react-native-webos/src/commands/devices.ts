import { COMMON_OPTIONS, parseCommandArgs, type Command } from '../cli/command.ts';
import { Reporter } from '../cli/reporter.ts';
import { probeTv, type TvProbe } from '../discovery/probe-tv.ts';
import { scanForTvs } from '../discovery/scan-tvs.ts';
import { readKnownDevices } from '../webos/known-devices.ts';
import { printTv, unseenTv } from './devices-report.ts';

const OPTIONS = {
  'no-scan': {
    type: 'boolean',
    description: 'Only probe the TVs connected before, without scanning the network',
  },
  ...COMMON_OPTIONS,
} as const;

const plural = (count: number) => `${count} LG ${count === 1 ? 'TV' : 'TVs'}`;

/** A TV the scan missed still has Developer Mode on when `rn-webos` logs in to it. */
function withLogin(tv: TvProbe): TvProbe {
  return tv.connection.state === 'ready' ? { ...tv, developerMode: true } : tv;
}

/** `rn-webos devices`: finds the LG TVs on the network and the ones connected before, and probes each. */
export const devicesCommand: Command = {
  name: 'devices',
  summary: 'Find LG TVs on the network and check Developer Mode, the key server and the login',
  options: OPTIONS,
  examples: ['rn-webos devices', 'rn-webos devices --no-scan'],
  async run(args) {
    const values = parseCommandArgs('devices', args, OPTIONS);
    const reporter = new Reporter({ verbose: values.verbose ?? false });
    const found =
      values['no-scan'] === true
        ? []
        : await reporter.step(
            'Scan',
            (task) =>
              scanForTvs({ onProgress: (done, total) => task.progress(`${done}/${total} hosts`) }),
            (tvs) => plural(tvs.length),
          );
    const hosts = new Set(found.map((tv) => tv.host));
    const known = (await readKnownDevices()).filter((tv) => !hosts.has(tv.host));
    const candidates = [...found, ...known.map((tv) => unseenTv(tv.host, tv.port))];
    if (candidates.length === 0) {
      reporter.line('No LG TV found.');
      reporter.note(
        'Is the TV on, and on this network? Connect one by address with: rn-webos connect <tv ip>',
      );
      return;
    }
    const probes = await reporter.step(
      'Probe',
      async () => (await Promise.all(candidates.map(probeTv))).map(withLogin),
      (tvs) =>
        `${tvs.filter((tv) => tv.connection.state === 'ready').length} of ${tvs.length} connected`,
    );
    for (const tv of probes) {
      reporter.line();
      printTv(reporter, tv);
    }
  },
};
