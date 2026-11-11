import { COMMON_OPTIONS, parseCommandArgs, type Command } from '../cli/command.ts';
import { Reporter } from '../cli/reporter.ts';
import { probeTv } from '../discovery/probe-tv.ts';
import { scanForTvs, type FoundTv } from '../discovery/scan-tvs.ts';
import { tcpOpen } from '../discovery/tcp-probe.ts';
import { KEY_SERVER_PORT } from '../webos/key-server.ts';
import { readKnownDevices, type KnownDevice } from '../webos/known-devices.ts';
import { printTv } from './devices-report.ts';

const OPTIONS = {
  'no-scan': {
    type: 'boolean',
    description: 'Only probe the TVs connected before, without scanning the network',
  },
  ...COMMON_OPTIONS,
} as const;

const plural = (count: number) => `${count} LG ${count === 1 ? 'TV' : 'TVs'}`;

const PROBE_TIMEOUT_MS = 1000;

/** A TV `rn-webos` was connected to that the scan did not find, with what its two Developer Mode ports say. */
async function unseenTv(tv: KnownDevice): Promise<FoundTv> {
  const [developerMode, keyServer] = await Promise.all([
    tcpOpen(tv.host, tv.port, PROBE_TIMEOUT_MS),
    tcpOpen(tv.host, KEY_SERVER_PORT, PROBE_TIMEOUT_MS),
  ]);
  return {
    host: tv.host,
    name: '',
    model: '',
    id: null,
    webosVersion: null,
    developerMode,
    keyServer,
    sshPort: tv.port,
  };
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
    const candidates = [...found, ...(await Promise.all(known.map(unseenTv)))];
    if (candidates.length === 0) {
      reporter.line('No LG TV found.');
      reporter.note(
        'Is the TV on, and on this network? Connect one by address with: rn-webos connect <tv ip>',
      );
      return;
    }
    const probes = await reporter.step(
      'Probe',
      () => Promise.all(candidates.map(probeTv)),
      (tvs) =>
        `${tvs.filter((tv) => tv.connection.state === 'ready').length} of ${tvs.length} connected`,
    );
    for (const tv of probes) {
      reporter.line();
      printTv(reporter, tv);
    }
  },
};
