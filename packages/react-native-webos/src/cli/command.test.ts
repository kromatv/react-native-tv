import { describe, expect, it } from 'vitest';
import { COMMON_OPTIONS, parseCommandArgs, UsageError, type OptionSpecs } from './command.ts';

const OPTIONS = {
  ...COMMON_OPTIONS,
  arch: { type: 'string', description: 'CPU' },
  release: { type: 'boolean', description: 'Release build' },
} as const satisfies OptionSpecs;

describe('parseCommandArgs', () => {
  it('reads long, short and boolean options', () => {
    expect(parseCommandArgs('package', ['--arch', 'aarch64', '-v', '--release'], OPTIONS)).toEqual({
      arch: 'aarch64',
      verbose: true,
      release: true,
    });
  });

  it('reads an option given as --name=value', () => {
    expect(parseCommandArgs('package', ['--arch=armv7l'], OPTIONS).arch).toBe('armv7l');
  });

  it('suggests the option a typo was meant to be', () => {
    expect(() => parseCommandArgs('package', ['--arc', 'armv7l'], OPTIONS)).toThrow(
      'Unknown option --arc. Did you mean --arch?',
    );
  });

  it('suggests nothing for an option far from every known one', () => {
    expect(() => parseCommandArgs('package', ['--fast'], OPTIONS)).toThrow(
      /^Unknown option --fast$/,
    );
  });

  it('refuses a positional argument as a usage error of the command', () => {
    const parse = () => parseCommandArgs('package', ['android'], OPTIONS);

    expect(parse).toThrow(UsageError);
    expect(parse).toThrow("Unexpected argument 'android'");
  });

  it('refuses an option missing its value', () => {
    expect(() => parseCommandArgs('package', ['--arch'], OPTIONS)).toThrow(UsageError);
  });
});
