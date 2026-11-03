import { COMMON_OPTIONS, parseCommandArgs, type Command } from '../cli/command.ts';
import { Reporter } from '../cli/reporter.ts';
import { FixableError } from '../errors.ts';
import { parseDeviceAddress } from '../tizen/device-address.ts';
import { connectDevice, listDevices, type Device } from '../tizen/devices.ts';

const OPTIONS = {
  connect: {
    type: 'string',
    placeholder: '<ip>',
    description: 'Connect to the TV at this address first, and remember it',
  },
  ...COMMON_OPTIONS,
} as const;

function describe(device: Device): string {
  const { cpuArch, platformVersion } = device.capabilities;
  return `${cpuArch}, Tizen ${platformVersion}`;
}

/** `rn-tizen devices`: probes the TVs reached before, with their CPU and platform. */
export const devicesCommand: Command = {
  name: 'devices',
  summary: 'List the TVs reached before, with their CPU and Tizen version',
  options: OPTIONS,
  examples: ['rn-tizen devices', 'rn-tizen devices --connect 192.168.1.50'],
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
      await reporter.step(`Connect to ${values.connect}`, () => connectDevice(address), describe);
    }
    const statuses = await listDevices();
    if (statuses.length === 0) {
      reporter.line('No TV is known yet.');
      reporter.note('Connect one with: rn-tizen devices --connect <tv ip>');
      return;
    }
    for (const status of statuses) {
      if (status.state === 'offline') {
        reporter.line(
          `${status.serial}  ${reporter.paint('yellow', 'offline')}  ${reporter.paint('gray', status.reason)}`,
        );
        continue;
      }
      const name = status.device.name === '' ? '' : `${status.device.name}  `;
      reporter.line(
        `${reporter.paint('bold', status.serial)}  ${name}${reporter.paint('gray', describe(status.device))}`,
      );
    }
  },
};
