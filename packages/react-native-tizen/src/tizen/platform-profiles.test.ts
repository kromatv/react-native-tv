import { describe, expect, it } from 'vitest';
import { FixableError } from '../errors.ts';
import {
  TIZENFX_API_LEVELS,
  newestPlatformFor,
  platformProfile,
  profileFor,
  runsOn,
} from './platform-profiles.ts';

describe('platformProfile', () => {
  it('builds a Tizen 9.0 package against TizenFX API 9 with Tizen.NET 12', () => {
    expect(platformProfile('9.0')).toEqual({
      platformVersion: '9.0',
      tizenFxApiLevel: 9,
      targetFramework: 'tizen90',
      tizenNetVersion: '12.0.0.18510',
      apiVersion: '9.0',
    });
  });

  it('refuses a platform nobody has verified, naming the supported ones', () => {
    expect(() => platformProfile('8.0')).toThrow(FixableError);
    expect(() => platformProfile('8.0')).toThrow('Tizen 8.0 is not a supported platform');
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

describe('newestPlatformFor', () => {
  it('picks the platform a TV runs when it is supported', () => {
    expect(newestPlatformFor('9.0')).toBe('9.0');
  });

  it('picks an older supported platform for a newer TV', () => {
    expect(newestPlatformFor('10.0')).toBe('9.0');
  });

  it('finds nothing for a TV older than every supported platform', () => {
    expect(newestPlatformFor('6.5')).toBeNull();
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
