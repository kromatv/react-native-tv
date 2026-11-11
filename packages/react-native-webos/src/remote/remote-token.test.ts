import { existsSync, mkdirSync, mkdtempSync, rmSync, statSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, describe, expect, it } from 'vitest';
import { readToken, saveToken, tokenPath } from './remote-token.ts';

const dirs: string[] = [];

function cacheEnv(): NodeJS.ProcessEnv {
  const dir = mkdtempSync(join(tmpdir(), 'rn-tizen-token-'));
  dirs.push(dir);
  return { RN_TIZEN_CACHE_DIR: join(dir, 'cache'), XDG_STATE_HOME: join(dir, 'state') };
}

afterEach(() => {
  for (const dir of dirs.splice(0)) rmSync(dir, { recursive: true, force: true });
});

describe('remote tokens', () => {
  it('saves a token readable by this user only and reads it back', () => {
    const env = cacheEnv();

    saveToken('192.168.1.50', '87654321', env);

    expect(readToken('192.168.1.50', env)).toBe('87654321');
    expect(statSync(tokenPath('192.168.1.50', env)).mode & 0o777).toBe(0o600);
    expect(
      statSync(join(env.XDG_STATE_HOME ?? '', 'react-native-tizen', 'remote')).mode & 0o777,
    ).toBe(0o700);
  });

  it('keeps tokens in the state directory, out of the cache', () => {
    const env = cacheEnv();

    saveToken('192.168.1.50', '87654321', env);

    expect(tokenPath('192.168.1.50', env)).toBe(
      join(env.XDG_STATE_HOME ?? '', 'react-native-tizen', 'remote', '192.168.1.50.token'),
    );
    expect(existsSync(join(env.RN_TIZEN_CACHE_DIR ?? '', 'remote'))).toBe(false);
  });

  it('moves a token an older rn-tizen kept in the cache', () => {
    const env = cacheEnv();
    const cached = join(env.RN_TIZEN_CACHE_DIR ?? '', 'remote', '192.168.1.50.token');
    mkdirSync(join(env.RN_TIZEN_CACHE_DIR ?? '', 'remote'), { recursive: true });
    writeFileSync(cached, '87654321');

    const token = readToken('192.168.1.50', env);

    expect(token).toBe('87654321');
    expect(existsSync(cached)).toBe(false);
    expect(statSync(tokenPath('192.168.1.50', env)).mode & 0o777).toBe(0o600);
  });

  it('reads nothing for a TV that never paired', () => {
    expect(readToken('192.168.1.51', cacheEnv())).toBeNull();
  });

  it('reads nothing from a file that does not hold a token', () => {
    const env = cacheEnv();
    saveToken('192.168.1.50', '1', env);

    writeFileSync(tokenPath('192.168.1.50', env), 'not a token&x=1');

    expect(readToken('192.168.1.50', env)).toBeNull();
  });
});
