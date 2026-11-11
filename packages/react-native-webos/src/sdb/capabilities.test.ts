import { describe, expect, it } from 'vitest';
import { capabilityEntries, parseCapabilities } from './capabilities.ts';
import { capabilityPayload, TV_CAPABILITIES } from './fake-tv.fixtures.ts';

describe('parseCapabilities', () => {
  it('reads the CPU, platform, profile and tool path of a Tizen 9.0 TV', () => {
    expect(parseCapabilities(capabilityPayload(TV_CAPABILITIES))).toEqual({
      cpuArch: 'armv7',
      platformVersion: '9.0',
      profile: 'tv',
      toolPath: '/home/owner/share/tmp/sdk_tools',
    });
  });

  it('reads no further than the byte count the payload starts with', () => {
    const payload = Buffer.concat([
      capabilityPayload('cpu_arch:armv7\n'),
      Buffer.from('platform_version:9.0\n'),
    ]);

    expect(capabilityEntries(payload)).toEqual({ cpu_arch: 'armv7' });
    expect(parseCapabilities(payload)).toBeNull();
  });

  it('keeps everything after the first colon of a value', () => {
    expect(capabilityEntries(capabilityPayload('log_path:/tmp:x\n'))).toEqual({
      log_path: '/tmp:x',
    });
  });

  it('drops a tool path a shell could misread', () => {
    const payload = capabilityPayload(
      'cpu_arch:armv7\nplatform_version:9.0\nsdk_toolpath:/tmp/$(reboot)\n',
    );

    expect(parseCapabilities(payload)).toBeNull();
  });

  it('finds nothing in a payload too short for its byte count', () => {
    expect(capabilityEntries(Buffer.from([0x05]))).toEqual({});
  });
});
