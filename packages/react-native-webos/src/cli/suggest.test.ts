import { describe, expect, it } from 'vitest';
import { closestMatch } from './suggest.ts';

const COMMANDS = [
  'doctor',
  'setup',
  'devices',
  'build-runtime',
  'bundle',
  'package',
  'run',
  'logs',
];

describe('closestMatch', () => {
  it('finds the command a typo was meant to be', () => {
    expect(closestMatch('packge', COMMANDS)).toBe('package');
    expect(closestMatch('devcies', COMMANDS)).toBe('devices');
  });

  it('finds nothing for a word unlike every candidate', () => {
    expect(closestMatch('deploy', COMMANDS)).toBeNull();
  });
});
