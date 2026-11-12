import { describe, expect, it } from 'vitest';
import { FixableError } from '../errors.ts';
import {
  SUPPORTED_PLATFORMS,
  TIZENFX_API_LEVELS,
  platformProfile,
  profileFor,
  runsOn,
} from './platform-profiles.ts';

describe('platformProfile', () => {
  it('builds every floor against TizenFX API 8, the newest the shell calls', () => {
    expect(platformProfile('6.0')).toEqual({
      platformVersion: '6.0',
      tizenFxApiLevel: 8,
      targetFramework: 'tizen80',
      tizenNetVersion: '12.0.0.18510',
      apiVersion: '6.0',
    });
    expect(platformProfile('9.0')).toMatchObject({ targetFramework: 'tizen80', apiVersion: '9.0' });
  });

  it('takes every floor from 6.0 to 10.0', () => {
    expect(SUPPORTED_PLATFORMS.map((platform) => platformProfile(platform).apiVersion)).toEqual(
      SUPPORTED_PLATFORMS,
    );
  });

  it('refuses a floor below 6.0, pointing at the web build', () => {
    expect(() => platformProfile('5.5')).toThrow(FixableError);
    expect(() => platformProfile('5.5')).toThrow('older than Tizen 6.0');
  });

  it('refuses a floor no rootstrap exists for, naming the supported ones', () => {
    expect(() => platformProfile('7.5')).toThrow('Tizen 7.5 is not a supported platform');
  });
});

describe('profileFor', () => {
  it('takes the target framework from the API level, not from the platform version', () => {
    const profile = profileFor({
      platformVersion: '10.0',
      tizenFxApiLevel: 13,
      tizenNetVersion: '13.0.0',
    });

    expect(profile.targetFramework).toBe('net8.0-tizen10.0');
    expect(profile.apiVersion).toBe('10.0');
  });

  it('refuses an API level newer than the platform it is built for', () => {
    expect(() =>
      profileFor({ platformVersion: '9.0', tizenFxApiLevel: 13, tizenNetVersion: '13.0.0' }),
    ).toThrow('TizenFX API level 13 needs Tizen 10.0');
  });

  it('refuses an API level TizenFX never had', () => {
    expect(() =>
      profileFor({ platformVersion: '9.0', tizenFxApiLevel: 7, tizenNetVersion: '12.0.0' }),
    ).toThrow('No TizenFX API level 7');
  });
});

describe('TIZENFX_API_LEVELS', () => {
  it('maps tizen10.0 to Tizen 7.0, the name a version string would get wrong', () => {
    expect(TIZENFX_API_LEVELS[10]).toEqual({ targetFramework: 'tizen10.0', firstPlatform: '7.0' });
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
