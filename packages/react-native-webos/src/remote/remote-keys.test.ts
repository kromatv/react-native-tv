import { describe, expect, it } from 'vitest';
import { remoteButton } from './remote-keys.ts';

describe('remoteButton', () => {
  it("takes the simulator's key names in any case", () => {
    expect(['Down', 'enter', 'OK', 'Back', 'FastForward', 'Blue'].map(remoteButton)).toEqual([
      'DOWN',
      'ENTER',
      'ENTER',
      'BACK',
      'FASTFORWARD',
      'BLUE',
    ]);
  });

  it('takes digits', () => {
    expect(remoteButton('1')).toBe('1');
  });

  it('passes a button name in capitals through as written', () => {
    expect(remoteButton('VOLUMEUP')).toBe('VOLUMEUP');
    expect(remoteButton('3D_MODE')).toBe('3D_MODE');
  });

  it('says an LG TV has no play/pause toggle', () => {
    expect(() => remoteButton('PlayPause')).toThrow('no play/pause button');
  });

  it('refuses an unknown name and suggests the closest', () => {
    expect(() => remoteButton('Dwon')).toThrow('Unknown key Dwon. Did you mean down?');
  });
});
