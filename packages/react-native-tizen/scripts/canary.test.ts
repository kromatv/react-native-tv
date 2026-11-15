import { describe, expect, it } from 'vitest';
import { canaryVersion, stampCanary } from './canary.ts';

const SHA = '3615491a0b5c7d9e1f2a3b4c5d6e7f8091a2b3c4';

describe('canaryVersion', () => {
  it('numbers the build and names the commit', () => {
    expect(canaryVersion(42, SHA)).toBe('0.0.0-canary.42.g3615491');
  });

  it('keeps a short sha that starts with a zero a valid version', () => {
    expect(canaryVersion(7, '0123456')).toBe('0.0.0-canary.7.g0123456');
  });

  it('refuses a build number that is not a positive integer', () => {
    for (const build of [0, -1, 1.5, Number.NaN]) {
      expect(() => canaryVersion(build, SHA)).toThrow(/build number/);
    }
  });

  it('refuses anything that is not a commit', () => {
    for (const sha of ['', 'main', '3615491; rm', 'ABCDEF0']) {
      expect(() => canaryVersion(1, sha)).toThrow(/commit/);
    }
  });
});

describe('stampCanary', () => {
  const tv = JSON.stringify({
    name: '@kromatv/react-native-tv',
    version: '0.1.0-alpha.2',
    type: 'module',
    dependencies: {
      '@kromatv/react-native-tizen': '^0.1.0-alpha.5',
      '@kromatv/react-native-webos': '^0.1.0-alpha.3',
      zod: '^4.6.7',
    },
  });

  it('pins the other canary packages to the same canary and leaves the rest', () => {
    const stamped = JSON.parse(stampCanary(tv, '0.0.0-canary.42.g3615491'));

    expect(stamped.version).toBe('0.0.0-canary.42.g3615491');
    expect(stamped.dependencies).toEqual({
      '@kromatv/react-native-tizen': '0.0.0-canary.42.g3615491',
      '@kromatv/react-native-webos': '0.0.0-canary.42.g3615491',
      zod: '^4.6.7',
    });
  });

  it('keeps the field order and ends the file with a newline', () => {
    const stamped = stampCanary(tv, '0.0.0-canary.1.g3615491');

    expect(Object.keys(JSON.parse(stamped))).toEqual(['name', 'version', 'type', 'dependencies']);
    expect(stamped.endsWith('}\n')).toBe(true);
  });

  it('stamps a manifest with no dependencies without adding any', () => {
    const stamped = JSON.parse(
      stampCanary(
        '{"name":"@kromatv/react-native-tizen","version":"0.1.0-alpha.6"}',
        '0.0.0-canary.1.g3615491',
      ),
    );

    expect(stamped).toEqual({
      name: '@kromatv/react-native-tizen',
      version: '0.0.0-canary.1.g3615491',
    });
  });
});
