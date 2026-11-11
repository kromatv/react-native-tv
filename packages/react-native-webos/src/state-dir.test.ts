import { homedir } from 'node:os';
import { join } from 'node:path';
import { describe, expect, it } from 'vitest';
import { stateDir } from './state-dir.ts';

describe('stateDir', () => {
  it('lives under XDG_STATE_HOME when it is set', () => {
    expect(stateDir({ XDG_STATE_HOME: '/state' })).toBe(join('/state', 'react-native-tizen'));
  });

  it('defaults to ~/.local/state/react-native-tizen', () => {
    expect(stateDir({})).toBe(join(homedir(), '.local', 'state', 'react-native-tizen'));
  });
});
