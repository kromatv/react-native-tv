import { homedir } from 'node:os';
import { join } from 'node:path';

/**
 * Where `rn-tizen` keeps what a cache wipe must not lose, such as the token a TV
 * issued when it paired: `$XDG_STATE_HOME/react-native-tizen`, else
 * `~/.local/state/react-native-tizen`.
 */
export function stateDir(env: NodeJS.ProcessEnv = process.env): string {
  if (env.XDG_STATE_HOME) return join(env.XDG_STATE_HOME, 'react-native-tizen');
  return join(homedir(), '.local', 'state', 'react-native-tizen');
}
