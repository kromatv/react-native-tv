import { describe, expect, it } from 'vitest';
import { runtimeConfig } from './runtime-config.ts';

const app = { packageId: 'org.example.app' };

describe('runtimeConfig', () => {
  it('marks a development build and points it at the log listener', () => {
    expect(runtimeConfig('index.webos.hbc', { host: '192.168.1.20', port: 8099 }, app)).toEqual({
      appKey: 'main',
      packageId: 'org.example.app',
      bundleFile: 'index.webos.hbc',
      development: true,
      log: { host: '192.168.1.20', port: 8099 },
    });
  });

  it('marks a release build, which streams no logs', () => {
    expect(runtimeConfig('index.webos.hbc', null, app)).toEqual({
      appKey: 'main',
      packageId: 'org.example.app',
      bundleFile: 'index.webos.hbc',
      development: false,
    });
  });

  it('names the dev server a build loads its bundle from', () => {
    const url = 'http://192.168.1.20:8081/.expo/.virtual-metro-entry.bundle?platform=webos';

    expect(runtimeConfig('index.webos.hbc', { host: 'h', port: 8099 }, app, url).bundleUrl).toBe(
      url,
    );
  });

  it('refuses a port the host would refuse', () => {
    expect(() => runtimeConfig('index.webos.hbc', { host: 'h', port: 70000 }, app)).toThrow();
  });
});
