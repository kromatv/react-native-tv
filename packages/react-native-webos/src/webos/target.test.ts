import { describe, expect, it } from 'vitest';
import { archFromDevice, compareWebOSVersions, rootstrapFor } from './target.ts';

describe('rootstrapFor', () => {
  it('links a floor from 8.0 against the webos profile of that version', () => {
    expect(rootstrapFor({ arch: 'armv7l', platformVersion: '9.0' })).toEqual({
      profile: 'webos',
      platformVersion: '9.0',
      name: 'webos-9.0-device.core',
      sdkPackage: 'WEBOS-9.0-NativeAppDevelopment-CLI',
    });
  });

  it('links an older floor against IoT Headed, the profile with armv7 and aarch64', () => {
    expect(rootstrapFor({ arch: 'armv7l', platformVersion: '6.0' })).toEqual({
      profile: 'iot-headed',
      platformVersion: '6.0',
      name: 'iot-headed-6.0-device.core',
      sdkPackage: 'IOT-Headed-6.0-NativeAppDevelopment-CLI',
    });
    expect(rootstrapFor({ arch: 'aarch64', platformVersion: '6.5' }).name).toBe(
      'iot-headed-6.5-device64.core',
    );
  });

  it('links the emulator against 10.0, the one x86_64 TV emulator, whatever the floor', () => {
    expect(rootstrapFor({ arch: 'x86_64', platformVersion: '6.0' })).toMatchObject({
      platformVersion: '10.0',
      name: 'webos-10.0-emulator64.core',
    });
    expect(rootstrapFor({ arch: 'x86_64', platformVersion: '9.0' }).name).toBe(
      'webos-10.0-emulator64.core',
    );
  });
});

describe('compareWebOSVersions', () => {
  it('orders by major, then minor, as numbers', () => {
    expect(compareWebOSVersions('6.5', '10.0')).toBeLessThan(0);
    expect(compareWebOSVersions('9.0', '9.0')).toBe(0);
    expect(compareWebOSVersions('6.5', '6.0')).toBeGreaterThan(0);
    expect(compareWebOSVersions('9', '9.0')).toBe(0);
  });
});

describe('archFromDevice', () => {
  it('maps what sdb reports to a build arch', () => {
    expect(archFromDevice(' armv7l ')).toBe('armv7l');
    expect(archFromDevice('arm64')).toBe('aarch64');
    expect(archFromDevice('mips')).toBeNull();
  });
});
