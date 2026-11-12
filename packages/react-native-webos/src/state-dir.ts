import { homedir } from 'node:os';
import { join } from 'node:path';

/**
 * Where `rn-webos` keeps what a cache wipe must not lose, such as the token a TV
 * issued when it paired: `$XDG_STATE_HOME/react-native-webos`, else
 * `~/.local/state/react-native-webos`.
 */
export function stateDir(env: NodeJS.ProcessEnv = process.env): string {
  if (env.XDG_STATE_HOME) return join(env.XDG_STATE_HOME, 'react-native-webos');
  return join(homedir(), '.local', 'state', 'react-native-webos');
}
