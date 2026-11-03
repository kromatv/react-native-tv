import { COMMON_OPTIONS, parseCommandArgs, type Command } from '../cli/command.ts';
import { Reporter } from '../cli/reporter.ts';
import { connectDevice, listDevices, readCapabilities } from '../tizen/sdb.ts';
import { deviceSerial } from '../tizen/sdb-output.ts';
import { locateTizenSdk } from '../tizen/tizen-sdk.ts';

const OPTIONS = {
  connect: {
    type: 'string',
    placeholder: '<ip>',
    description: 'Connect to the TV at this address first',
  },
  ...COMMON_OPTIONS,
} as const;

/** `rn-tizen devices`: lists the TVs sdb can reach, with their CPU and platform. */
export const devicesCommand: Command = {
  name: 'devices',
  summary: 'List the TVs sdb can reach, with their CPU and Tizen version',
  options: OPTIONS,
  examples: ['rn-tizen devices', 'rn-tizen devices --connect 192.168.1.50'],
  async run(args) {
    const values = parseCommandArgs('devices', args, OPTIONS);
    const reporter = new Reporter({ verbose: values.verbose ?? false });
    const sdk = locateTizenSdk();
    if (values.connect !== undefined) {
      const serial = deviceSerial(values.connect);
      await reporter.step(`Connect to ${serial}`, (task) => connectDevice(task, sdk, serial));
    }
    const devices = await listDevices(reporter.task(), sdk);
    if (devices.length === 0) {
      reporter.line('No TV is connected.');
      reporter.note('Connect one with: rn-tizen devices --connect <tv ip>');
      return;
    }
    for (const device of devices) {
      const name = device.name === '' ? '' : `${device.name}  `;
      if (device.state !== 'device') {
        reporter.line(`${device.serial}  ${name}${reporter.paint('yellow', device.state)}`);
        continue;
      }
      const capabilities = await readCapabilities(reporter.task(), sdk, device.serial);
      const detail = `${capabilities.cpuArch}, Tizen ${capabilities.platformVersion}`;
      reporter.line(
        `${reporter.paint('bold', device.serial)}  ${name}${reporter.paint('gray', detail)}`,
      );
    }
  },
};
