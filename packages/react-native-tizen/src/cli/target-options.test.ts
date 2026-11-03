import { describe, expect, it } from 'vitest';
import { FixableError } from '../errors.ts';
import { UsageError } from './command.ts';
import { chooseTarget } from './target-options.ts';

const TV = { cpuArch: 'armv7', platformVersion: '9.0', profile: 'tv' };

describe('chooseTarget', () => {
  it('builds for armv7l on Tizen 9.0 when nothing says otherwise', () => {
    expect(chooseTarget({ command: 'package' })).toEqual({
      arch: 'armv7l',
      platformVersion: '9.0',
    });
  });

  it('takes the arch and platform the TV reports', () => {
    const device = { cpuArch: 'aarch64', platformVersion: '9.0', profile: 'tv' };

    expect(chooseTarget({ command: 'run', device })).toEqual({
      arch: 'aarch64',
      platformVersion: '9.0',
    });
  });

  it('builds a newer TV the newest supported platform', () => {
    const device = { ...TV, platformVersion: '10.0' };

    expect(chooseTarget({ command: 'run', device }).platformVersion).toBe('9.0');
  });

  it('lets the app config platform win over the TV', () => {
    const device = { ...TV, platformVersion: '10.0' };

    expect(chooseTarget({ command: 'run', configPlatform: '9.0', device }).platformVersion).toBe(
      '9.0',
    );
  });

  it('lets flags win over everything', () => {
    expect(
      chooseTarget({ command: 'package', arch: 'x86_64', platform: '9.0', configPlatform: '10.0' }),
    ).toEqual({ arch: 'x86_64', platformVersion: '9.0' });
  });

  it('refuses an unknown arch flag as a usage error', () => {
    expect(() => chooseTarget({ command: 'package', arch: 'arm64' })).toThrow(UsageError);
  });

  it('refuses a platform flag that is not a version', () => {
    expect(() => chooseTarget({ command: 'package', platform: 'nine' })).toThrow(UsageError);
  });

  it('refuses a target newer than the TV, which could not install it', () => {
    const device = { ...TV, platformVersion: '8.0' };

    expect(() => chooseTarget({ command: 'run', platform: '9.0', device })).toThrow(FixableError);
  });

  it('refuses a TV older than every supported platform', () => {
    const device = { ...TV, platformVersion: '6.5' };

    expect(() => chooseTarget({ command: 'run', device })).toThrow('older than every supported');
  });
});
