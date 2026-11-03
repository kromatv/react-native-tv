import { COMMON_OPTIONS, parseCommandArgs, type Command } from '../cli/command.ts';
import { Reporter } from '../cli/reporter.ts';
import { probeTv, type TvProbe } from '../discovery/probe-tv.ts';
import { scanForTvs, type FoundTv } from '../discovery/scan-tvs.ts';
import { FixableError } from '../errors.ts';
import { SDB_PORT, type SdbAddress } from '../sdb/sdb-connection.ts';
import { parseDeviceAddress } from '../tizen/device-address.ts';
import { connectDevice } from '../tizen/devices.ts';
import { readKnownDevices, rememberDevices } from '../tizen/known-devices.ts';

const OPTIONS = {
  connect: {
    type: 'string',
    placeholder: '<ip>',
    description: 'Connect to the TV at this address first, and remember it',
  },
  'no-scan': {
    type: 'boolean',
    description: 'Only probe the TVs reached before, without scanning the network',
  },
  ...COMMON_OPTIONS,
} as const;

function unknownTv({ host, port }: SdbAddress): FoundTv {
  return { host, name: '', model: '', developerMode: null, developerIp: null, sdbPort: port };
}

function developerMode(tv: TvProbe): string {
  if (tv.developerMode === null) return 'unknown: the TV did not answer on port 8001';
  if (!tv.developerMode) return 'off: on the TV, open Apps and type 12345 on the remote';
  if (tv.developerIp === null) return 'on, no host PC set';
  const who = tv.hostPcIsThisComputer === true ? 'this computer' : 'not this computer';
  return `on, host PC ${tv.developerIp} (${who})`;
}

function sdb(tv: TvProbe): string {
  const { sdb: state } = tv;
  if (state.state === 'ready') {
    const capabilities = state.capabilities;
    return capabilities === null
      ? 'ready'
      : `ready: ${capabilities.cpuArch}, Tizen ${capabilities.platformVersion}`;
  }
  if (state.state === 'refused')
    return 'busy: another computer or Tizen Studio holds its sdb connection';
  if (state.state === 'closed')
    return `closed: port ${SDB_PORT} opens once Developer mode is on and the TV restarted`;
  return 'accepted the connection but never answered';
}

function print(reporter: Reporter, tv: TvProbe): void {
  const title = [tv.name, tv.model].filter((part) => part !== '').join('  ');
  reporter.line(`${reporter.paint('bold', tv.host)}  ${title}`);
  reporter.field('Developer mode', developerMode(tv));
  reporter.field('sdb', sdb(tv));
  if (tv.sdb.state === 'ready' && tv.sdb.duid !== null) reporter.field('DUID', tv.sdb.duid);
}

/** `rn-tizen devices`: finds the Samsung TVs on the network and the ones reached before, and probes each. */
export const devicesCommand: Command = {
  name: 'devices',
  summary: 'Find Samsung TVs on the network and check Developer mode, sdb and the DUID of each',
  options: OPTIONS,
  examples: [
    'rn-tizen devices',
    'rn-tizen devices --connect 192.168.1.50',
    'rn-tizen devices --no-scan',
  ],
  async run(args) {
    const values = parseCommandArgs('devices', args, OPTIONS);
    const reporter = new Reporter({ verbose: values.verbose ?? false });
    if (values.connect !== undefined) {
      const address = parseDeviceAddress(values.connect);
      if (address === null) {
        throw new FixableError(
          `${values.connect} is not a TV address`,
          'Pass an IP address, such as 192.168.1.50.',
        );
      }
      await reporter.step(
        `Connect to ${values.connect}`,
        () => connectDevice(address),
        (tv) => tv.name || tv.serial,
      );
    }
    const found =
      values['no-scan'] === true
        ? []
        : await reporter.step(
            'Scan',
            (task) =>
              scanForTvs({ onProgress: (done, total) => task.progress(`${done}/${total} hosts`) }),
            (tvs) => `${tvs.length} Samsung ${tvs.length === 1 ? 'TV' : 'TVs'}`,
          );
    const hosts = new Set(found.map((tv) => tv.host));
    const known = (await readKnownDevices()).filter((address) => !hosts.has(address.host));
    const candidates = [...found, ...known.map(unknownTv)];
    if (candidates.length === 0) {
      reporter.line('No Samsung TV found.');
      reporter.note('Connect one by address with: rn-tizen devices --connect <tv ip>');
      return;
    }
    const probes = await reporter.step(
      'Probe',
      () => Promise.all(candidates.map(probeTv)),
      (tvs) =>
        `${tvs.filter((tv) => tv.sdb.state === 'ready').length} of ${tvs.length} ready for sdb`,
    );
    for (const tv of probes) {
      reporter.line();
      print(reporter, tv);
    }
    const ready = probes.filter((tv) => tv.sdb.state === 'ready');
    await rememberDevices(ready.map((tv) => ({ host: tv.host, port: tv.sdbPort })));
  },
};
