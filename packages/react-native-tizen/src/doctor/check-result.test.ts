import { describe, expect, it } from 'vitest';
import { FixableError } from '../errors.ts';
import { checking, isAtLeastVersion, parseVersion } from './check-result.ts';

describe('parseVersion', () => {
  it('reads the version out of a --version banner', () => {
    expect(parseVersion('cmake version 4.4.4\n\nCMake suite maintained by Kitware')).toBe('4.4.4');
    expect(parseVersion('Homebrew clang version 21.1.8')).toBe('21.1.8');
    expect(parseVersion('v26.5.0')).toBe('26.5.0');
  });

  it('finds nothing in output without a version', () => {
    expect(parseVersion('command not found')).toBeNull();
  });
});

describe('isAtLeastVersion', () => {
  it('compares each part as a number', () => {
    expect(isAtLeastVersion('3.24.0', '3.24')).toBe(true);
    expect(isAtLeastVersion('3.100', '3.24')).toBe(true);
    expect(isAtLeastVersion('3.9.2', '3.24')).toBe(false);
    expect(isAtLeastVersion('10.0.107', '6.0')).toBe(true);
  });
});

describe('checking', () => {
  it('turns a fixable error into a failed check that carries the fix', async () => {
    const result = await checking('TVs', () => {
      throw new FixableError('No TV is connected', 'Connect one');
    });

    expect(result).toEqual({
      title: 'TVs',
      status: 'fail',
      detail: 'No TV is connected',
      fix: 'Connect one',
    });
  });
});
