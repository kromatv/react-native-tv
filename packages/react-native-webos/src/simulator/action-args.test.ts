import { describe, expect, it } from 'vitest';
import { splitActionArgs } from './action-args.ts';

const SPECS = {
  verbose: { type: 'boolean', short: 'v', description: '' },
  out: { type: 'string', description: '' },
  instance: { type: 'string', description: '' },
  lines: { type: 'string', short: 'n', description: '' },
} as const;

describe('splitActionArgs', () => {
  it('keeps every key before the first option as a positional', () => {
    expect(splitActionArgs(['Down', 'Down', 'Enter', '--verbose'], SPECS)).toEqual({
      positionals: ['Down', 'Down', 'Enter'],
      options: ['--verbose'],
    });
  });

  it('leaves an option value with its option', () => {
    expect(splitActionArgs(['--out', 'shot.png'], SPECS)).toEqual({
      positionals: [],
      options: ['--out', 'shot.png'],
    });
  });

  it('takes what follows an option and its value as a positional again', () => {
    expect(
      splitActionArgs(['--instance', 'tv', 'Down', '-n', '5', 'Enter', '-v', 'Back'], SPECS),
    ).toEqual({
      positionals: ['Down', 'Enter', 'Back'],
      options: ['--instance', 'tv', '-n', '5', '-v'],
    });
  });

  it('reads a value written with = from the option itself, and gives an unknown option no value', () => {
    expect(splitActionArgs(['--instance=tv', 'Down', '--nope', 'Enter'], SPECS)).toEqual({
      positionals: ['Down', 'Enter'],
      options: ['--instance=tv', '--nope'],
    });
  });

  it('takes a negative number as a positional', () => {
    expect(splitActionArgs(['-3', '--verbose'], SPECS)).toEqual({
      positionals: ['-3'],
      options: ['--verbose'],
    });
  });

  it('takes a deep link with a query as one positional', () => {
    expect(splitActionArgs(['myapp://item/42?ref=home'], SPECS).positionals).toEqual([
      'myapp://item/42?ref=home',
    ]);
  });
});
