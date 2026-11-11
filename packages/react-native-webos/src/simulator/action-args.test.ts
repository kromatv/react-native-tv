import { describe, expect, it } from 'vitest';
import { splitActionArgs } from './action-args.ts';

describe('splitActionArgs', () => {
  it('keeps every key before the first option as a positional', () => {
    expect(splitActionArgs(['Down', 'Down', 'Enter', '--verbose'])).toEqual({
      positionals: ['Down', 'Down', 'Enter'],
      options: ['--verbose'],
    });
  });

  it('leaves an option value with its option', () => {
    expect(splitActionArgs(['--out', 'shot.png'])).toEqual({
      positionals: [],
      options: ['--out', 'shot.png'],
    });
  });

  it('takes a deep link with a query as one positional', () => {
    expect(splitActionArgs(['myapp://item/42?ref=home']).positionals).toEqual([
      'myapp://item/42?ref=home',
    ]);
  });
});
