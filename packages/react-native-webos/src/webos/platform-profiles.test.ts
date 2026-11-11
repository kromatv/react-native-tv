import { describe, expect, it } from 'vitest';
import { FixableError } from '../errors.ts';
import {
  SUPPORTED_PLATFORMS,
  WEBOSFX_API_LEVELS,
  platformProfile,
  profileFor,
  runsOn,
} from './platform-profiles.ts';

describe('platformProfile', () => {
  it('builds every floor against WebOSFX API 8, the newest the shell calls', () => {
    expect(platformProfile('6.0')).toEqual({
      platformVersion: '6.0',
      webosFxApiLevel: 8,
      targetFramework: 'webos80',
      webosNetVersion: '12.0.0.18510',
      apiVersion: '6.0',
    });
    expect(platformProfile('9.0')).toMatchObject({ targetFramework: 'webos80', apiVersion: '9.0' });
  });

  it('takes every floor from 6.0 to 10.0', () => {
    expect(SUPPORTED_PLATFORMS.map((platform) => platformProfile(platform).apiVersion)).toEqual(
      SUPPORTED_PLATFORMS,
    );
  });

  it('refuses a floor below 6.0, pointing at the web build', () => {
    expect(() => platformProfile('5.5')).toThrow(FixableError);
    expect(() => platformProfile('5.5')).toThrow('older than webOS 6.0');
  });

  it('refuses a floor no rootstrap exists for, naming the supported ones', () => {
    expect(() => platformProfile('7.5')).toThrow('webOS 7.5 is not a supported platform');
  });
});

describe('profileFor', () => {
  it('takes the target framework from the API level, not from the platform version', () => {
    const profile = profileFor({
      platformVersion: '10.0',
      webosFxApiLevel: 13,
      webosNetVersion: '13.0.0',
    });

    expect(profile.targetFramework).toBe('net8.0-webos10.0');
    expect(profile.apiVersion).toBe('10.0');
  });

  it('refuses an API level newer than the platform it is built for', () => {
    expect(() =>
      profileFor({ platformVersion: '9.0', webosFxApiLevel: 13, webosNetVersion: '13.0.0' }),
    ).toThrow('WebOSFX API level 13 needs webOS 10.0');
  });

  it('refuses an API level WebOSFX never had', () => {
    expect(() =>
      profileFor({ platformVersion: '9.0', webosFxApiLevel: 7, webosNetVersion: '12.0.0' }),
    ).toThrow('No WebOSFX API level 7');
  });
});

describe('WEBOSFX_API_LEVELS', () => {
  it('maps webos10.0 to webOS 7.0, the name a version string would get wrong', () => {
    expect(WEBOSFX_API_LEVELS[10]).toEqual({ targetFramework: 'webos10.0', firstPlatform: '7.0' });
  });
});

describe('runsOn', () => {
  it('lets a package run on its own platform and later ones', () => {
    expect(runsOn('9.0', '9.0')).toBe(true);
    expect(runsOn('9.0', '10.0')).toBe(true);
  });

  it('keeps a package off a TV older than its platform', () => {
    expect(runsOn('9.0', '8.0')).toBe(false);
  });
});
