import { describe, expect, it } from 'vitest';
import { archFromDevice, compareTizenVersions, rootstrapFor } from './target.ts';

describe('rootstrapFor', () => {
  it('links a floor from 8.0 against the tizen profile of that version', () => {
    expect(rootstrapFor({ arch: 'armv7l', platformVersion: '9.0' })).toEqual({
      profile: 'tizen',
      platformVersion: '9.0',
      name: 'tizen-9.0-device.core',
      sdkPackage: 'TIZEN-9.0-NativeAppDevelopment-CLI',
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
      name: 'tizen-10.0-emulator64.core',
    });
    expect(rootstrapFor({ arch: 'x86_64', platformVersion: '9.0' }).name).toBe(
      'tizen-10.0-emulator64.core',
    );
  });
});

describe('compareTizenVersions', () => {
  it('orders by major, then minor, as numbers', () => {
    expect(compareTizenVersions('6.5', '10.0')).toBeLessThan(0);
    expect(compareTizenVersions('9.0', '9.0')).toBe(0);
    expect(compareTizenVersions('6.5', '6.0')).toBeGreaterThan(0);
    expect(compareTizenVersions('9', '9.0')).toBe(0);
  });
});

describe('archFromDevice', () => {
  it('maps what sdb reports to a build arch', () => {
    expect(archFromDevice(' armv7l ')).toBe('armv7l');
    expect(archFromDevice('arm64')).toBe('aarch64');
    expect(archFromDevice('mips')).toBeNull();
  });
});
