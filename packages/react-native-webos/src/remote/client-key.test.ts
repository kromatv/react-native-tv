import { mkdtempSync, rmSync, statSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, describe, expect, it } from 'vitest';
import { clientKeyPath, readClientKey, saveClientKey } from './client-key.ts';

const dirs: string[] = [];
const TV = '6f2c1c1e-8b0a-4c57-9d0e-2a7b3a4f5e61';
const KEY = '0f1e2d3c4b5a69788796a5b4c3d2e1f0';

function stateEnv(): NodeJS.ProcessEnv {
  const dir = mkdtempSync(join(tmpdir(), 'rn-webos-client-key-'));
  dirs.push(dir);
  return { XDG_STATE_HOME: dir };
}

afterEach(() => {
  for (const dir of dirs.splice(0)) rmSync(dir, { recursive: true, force: true });
});

describe('client keys', () => {
  it('are saved readable by this user only, under the TV id, and read back', () => {
    const env = stateEnv();

    saveClientKey(TV, KEY, env);

    expect(readClientKey(TV, env)).toBe(KEY);
    expect(clientKeyPath(TV, env)).toBe(
      join(env.XDG_STATE_HOME ?? '', 'react-native-webos', 'remote', `${TV}.key`),
    );
    expect(statSync(clientKeyPath(TV, env)).mode & 0o777).toBe(0o600);
    expect(statSync(join(clientKeyPath(TV, env), '..')).mode & 0o777).toBe(0o700);
  });

  it('read as none for a TV that never paired, or a file that holds no key', () => {
    const env = stateEnv();
    expect(readClientKey('192.168.1.51', env)).toBeNull();

    saveClientKey(TV, KEY, env);
    writeFileSync(clientKeyPath(TV, env), 'not a key&x=1');

    expect(readClientKey(TV, env)).toBeNull();
  });

  it('refuse an id that would leave the directory', () => {
    expect(() => clientKeyPath('../../etc/passwd')).toThrow('cannot name');
    expect(readClientKey('../x', stateEnv())).toBeNull();
  });
});
