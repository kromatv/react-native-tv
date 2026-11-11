import { join } from 'node:path';
import { describe, expect, it } from 'vitest';
import { FixableError } from '../src/errors.ts';
import { pathInside } from './path-inside.ts';

const repository = '/work/react-native-tizen';
const build = '/home/runner/rn-tizen-simulator/build/host-tests/abc';

describe('pathInside', () => {
  it('returns a path inside the repository or the build directory, made absolute', () => {
    expect(pathInside(join(repository, 'coverage/runtime.xml'), [repository, build])).toBe(
      '/work/react-native-tizen/coverage/runtime.xml',
    );
    expect(pathInside(`${build}/report.xml`, [repository, build])).toBe(`${build}/report.xml`);
  });

  it('refuses a path that climbs out of every root', () => {
    expect(() => pathInside(`${repository}/../../etc/passwd`, [repository, build])).toThrow(
      FixableError,
    );
    expect(() => pathInside('/tmp/runtime.xml', [repository])).toThrow(
      '/tmp/runtime.xml is outside /work/react-native-tizen',
    );
  });

  it('refuses a root itself and a sibling that only shares its prefix', () => {
    expect(() => pathInside(repository, [repository])).toThrow(FixableError);
    expect(() => pathInside(`${repository}-evil/runtime.xml`, [repository])).toThrow(FixableError);
  });
});
