import { describe, expect, it } from 'vitest';
import { remoteKeyCode } from './remote-keys.ts';

describe('remoteKeyCode', () => {
  it("takes the simulator's key names in any case", () => {
    expect(
      ['Down', 'enter', 'OK', 'Back', 'PlayPause', 'FastForward', 'Blue'].map(remoteKeyCode),
    ).toEqual([
      'KEY_DOWN',
      'KEY_ENTER',
      'KEY_ENTER',
      'KEY_RETURN',
      'KEY_PLAY_BACK',
      'KEY_FF',
      'KEY_CYAN',
    ]);
  });

  it('takes digits', () => {
    expect(remoteKeyCode('1')).toBe('KEY_1');
  });

  it('passes a KEY_ code through as written', () => {
    expect(remoteKeyCode('KEY_CH_LIST')).toBe('KEY_CH_LIST');
  });

  it('refuses an unknown name and suggests the closest', () => {
    expect(() => remoteKeyCode('Dwon')).toThrow('Unknown key Dwon. Did you mean down?');
  });
});
