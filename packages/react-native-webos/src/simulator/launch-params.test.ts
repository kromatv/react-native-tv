import { describe, expect, it } from 'vitest';
import { launchParams } from './launch-params.ts';

describe('launchParams', () => {
  it('passes nothing on when no parameters were given', () => {
    expect(launchParams(undefined)).toBeUndefined();
  });

  it('writes a JSON object as one compact argument', () => {
    expect(launchParams('{ "contentTarget": "myapp://item/42" }')).toBe(
      '{"contentTarget":"myapp://item/42"}',
    );
  });

  it.each(['[1]', '"text"', '42', 'nope', ''])('refuses %s, which is no JSON object', (json) => {
    expect(() => launchParams(json)).toThrow('--params takes a JSON object');
  });
});
