import { describe, expect, it } from 'vitest';
import type { TvFeatures } from '../config/tv-features.ts';
import { runtimeConfig } from './runtime-config.ts';

const features: TvFeatures = {
  voice: null,
  preview: null,
  resolution: '1080p',
  backgroundMedia: false,
  pictureInPicture: false,
};
const app = { packageId: 'org.example.app', features };

describe('runtimeConfig', () => {
  it('marks a development build and points it at the log listener', () => {
    expect(runtimeConfig('index.tizen.hbc', { host: '192.168.1.20', port: 8099 }, app)).toEqual({
      appKey: 'main',
      packageId: 'org.example.app',
      bundleFile: 'index.tizen.hbc',
      development: true,
      log: { host: '192.168.1.20', port: 8099 },
      resolution: '1080p',
      backgroundMedia: false,
    });
  });

  it('marks a release build, which streams no logs', () => {
    expect(runtimeConfig('index.tizen.hbc', null, app)).toEqual({
      appKey: 'main',
      packageId: 'org.example.app',
      bundleFile: 'index.tizen.hbc',
      development: false,
      resolution: '1080p',
      backgroundMedia: false,
    });
  });

  it('refuses a port the host would refuse', () => {
    expect(() => runtimeConfig('index.tizen.hbc', { host: 'h', port: 70000 }, app)).toThrow();
  });

  it('tells the host and the shell which TV features to start', () => {
    const config = runtimeConfig('index.tizen.hbc', null, {
      packageId: 'org.example.app',
      features: {
        ...features,
        voice: { searchUrl: 'myapp://search?q={query}' },
        preview: { kind: 'personal', refreshMinutes: 30 },
        resolution: '4k',
        backgroundMedia: true,
      },
    });

    expect(config).toMatchObject({
      resolution: '4k',
      backgroundMedia: true,
      voice: { searchUrl: 'myapp://search?q={query}' },
      preview: { kind: 'personal', refreshMinutes: 30, service: 'org.example.app.preview' },
    });
  });

  it('turns voice on with no search URL', () => {
    const config = runtimeConfig('index.tizen.hbc', null, {
      ...app,
      features: { ...features, voice: { searchUrl: null } },
    });

    expect(config.voice).toEqual({});
  });
});
