import { describe, expect, it } from 'vitest';
import { FixableError } from '../errors.ts';
import { CommandError, MissingProgramError } from '../exec.ts';
import { UsageError } from './command.ts';
import { describeError } from './describe-error.ts';

describe('describeError', () => {
  it('shows a fixable error with its hint and no stack', () => {
    expect(describeError(new FixableError('No TV is connected', 'Pass --device'))).toEqual({
      message: 'No TV is connected',
      details: [],
      hint: 'Pass --device',
      exitCode: 1,
    });
  });

  it('points a usage error at the command help and exits 2', () => {
    const report = describeError(new UsageError('Unknown option --arc', 'package'));

    expect(report.hint).toBe('rn-tizen package --help lists what it takes.');
    expect(report.exitCode).toBe(2);
  });

  it('shows the last lines a failed tool printed', () => {
    const output = Array.from({ length: 30 }, (_, index) => `line ${index}`).join('\n');
    const report = describeError(new CommandError(['/usr/bin/cmake', '--build', 'x'], 1, output));

    expect(report.message).toBe('cmake exited with 1');
    expect(report.details).toHaveLength(20);
    expect(report.details.at(-1)).toBe('line 29');
  });

  it('says how to install a program that is missing', () => {
    const report = describeError(new MissingProgramError('ninja'));

    expect(report.message).toBe('ninja is not installed or not on PATH');
    expect(report.hint).toMatch(/ninja/);
  });

  it('keeps the stack of an unexpected error, which is a bug', () => {
    const report = describeError(new TypeError('x is undefined'));

    expect(report.message).toBe('x is undefined');
    expect(report.details.length).toBeGreaterThan(0);
    expect(report.hint).toMatch(/bug/);
  });
});
