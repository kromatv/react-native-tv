import { describe, expect, it } from 'vitest';
import { deviceHost, deviceSerial, parseCapabilities, parseDeviceList } from './sdb-output.ts';

const CAPABILITY = `secure_protocol:enabled
cpu_arch:armv7
core_abi:arm_32
sdk_toolpath:/home/owner/share/tmp/sdk_tools
profile_name:tv
vendor_name:Samsung
platform_version:9.0
architecture:32
`;

describe('parseDeviceList', () => {
  it('reads every device sdb lists, model name included', () => {
    const output = 'List of devices attached \n192.168.1.50:26101 \tdevice    \tGQ75LS03DAUXZG\n';

    expect(parseDeviceList(output)).toEqual([
      { serial: '192.168.1.50:26101', state: 'device', name: 'GQ75LS03DAUXZG' },
    ]);
  });

  it('finds no device in an empty list', () => {
    expect(parseDeviceList('List of devices attached \n\n')).toEqual([]);
  });

  it('skips the notice sdb prints when it starts its daemon', () => {
    const output =
      '* Server is not running. Start it now on port 26099 *\n* Server has started successfully *\nList of devices attached \n';

    expect(parseDeviceList(output)).toEqual([]);
  });

  it('keeps a device sdb reports offline, with its state', () => {
    const output = 'List of devices attached \nemulator-26101\toffline\tT-samsung-9.0-x86\n';

    expect(parseDeviceList(output)[0]?.state).toBe('offline');
  });
});

describe('parseCapabilities', () => {
  it('reads the arch, platform and profile of a TV', () => {
    expect(parseCapabilities(CAPABILITY)).toEqual({
      cpuArch: 'armv7',
      platformVersion: '9.0',
      profile: 'tv',
    });
  });

  it('gives up when the platform version is missing', () => {
    expect(parseCapabilities('cpu_arch:armv7\n')).toBeNull();
  });

  it('keeps everything after the first colon of a value', () => {
    const output = 'cpu_arch:aarch64\nplatform_version:10.0\nlog_path:/tmp:x\n';

    expect(parseCapabilities(output)?.cpuArch).toBe('aarch64');
  });
});

describe('deviceSerial', () => {
  it('adds the sdb port to a bare IP address', () => {
    expect(deviceSerial('192.168.1.50')).toBe('192.168.1.50:26101');
  });

  it('keeps an explicit port and any other serial as typed', () => {
    expect(deviceSerial('192.168.1.50:26102')).toBe('192.168.1.50:26102');
    expect(deviceSerial('emulator-26101')).toBe('emulator-26101');
  });
});

describe('deviceHost', () => {
  it('reads the IP address of a network serial', () => {
    expect(deviceHost('192.168.1.50:26101')).toBe('192.168.1.50');
  });

  it('finds no host in an emulator serial', () => {
    expect(deviceHost('emulator-26101')).toBeNull();
  });
});
