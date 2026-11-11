import { describe, expect, it } from 'vitest';
import { resolveWebOSConfig } from './webos-config.ts';

const expo = (webos: Record<string, unknown>, extra: Record<string, unknown> = {}) => ({
  name: 'My App',
  version: '1.0.0',
  webos: { package: 'org.example.app', ...webos },
  ...extra,
});

describe('expo.webos.voice', () => {
  it('turns voice on and asks for Samsung voice privileges', () => {
    const config = resolveWebOSConfig(expo({ voice: true }));

    expect(config.features.voice).toEqual({ searchUrl: null });
    expect(config.privileges).toEqual([
      'http://webos.org/privilege/internet',
      'http://developer.samsung.com/privilege/voicecontrol',
      'http://developer.samsung.com/privilege/microphone',
    ]);
  });

  it('keeps a search URL with known placeholders', () => {
    const config = resolveWebOSConfig(
      expo({ voice: { searchUrl: 'myapp://search?q={query}&g={genre}' } }),
    );

    expect(config.features.voice).toEqual({ searchUrl: 'myapp://search?q={query}&g={genre}' });
  });

  it('refuses a search URL with an unknown placeholder', () => {
    expect(() =>
      resolveWebOSConfig(expo({ voice: { searchUrl: 'myapp://search?q={name}' } })),
    ).toThrow('expo.webos.voice.searchUrl: unknown placeholder');
  });

  it('refuses a search URL with no placeholder', () => {
    expect(() => resolveWebOSConfig(expo({ voice: { searchUrl: 'myapp://search' } }))).toThrow(
      'expected at least one placeholder',
    );
  });
});

describe('expo.webos.preview', () => {
  it('takes a public preview from an endpoint', () => {
    const config = resolveWebOSConfig(
      expo({ preview: { endpoint: 'https://example.com/preview.json' } }),
    );

    expect(config.features.preview).toEqual({
      kind: 'public',
      endpoint: 'https://example.com/preview.json',
    });
    expect(config.privileges).toEqual(['http://webos.org/privilege/internet']);
  });

  it('takes a personal preview and asks to launch its service app', () => {
    const config = resolveWebOSConfig(expo({ preview: { refreshMinutes: 30 } }));

    expect(config.features.preview).toEqual({ kind: 'personal', refreshMinutes: 30 });
    expect(config.privileges).toContain('http://webos.org/privilege/appmanager.launch');
  });

  it('refuses a refresh more often than Samsung allows', () => {
    expect(() => resolveWebOSConfig(expo({ preview: { refreshMinutes: 5 } }))).toThrow(
      'expo.webos.preview',
    );
  });
});

describe('expo.webos.resolution', () => {
  it('defaults to 1080p', () => {
    expect(resolveWebOSConfig(expo({})).features.resolution).toBe('1080p');
  });

  it('takes 4k', () => {
    expect(resolveWebOSConfig(expo({ resolution: '4k' })).features.resolution).toBe('4k');
  });

  it('refuses another resolution', () => {
    expect(() => resolveWebOSConfig(expo({ resolution: '8k' }))).toThrow('expo.webos.resolution');
  });
});

describe('expo-video config plugin options', () => {
  it('reads supportsBackgroundPlayback and supportsPictureInPicture', () => {
    const config = resolveWebOSConfig(
      expo(
        {},
        {
          plugins: [
            'expo-font',
            ['expo-video', { supportsBackgroundPlayback: true, supportsPictureInPicture: true }],
          ],
        },
      ),
    );

    expect(config.features).toMatchObject({ backgroundMedia: true, pictureInPicture: true });
  });

  it('turns nothing on for expo-video without options', () => {
    const config = resolveWebOSConfig(expo({}, { plugins: ['expo-video'] }));

    expect(config.features).toMatchObject({ backgroundMedia: false, pictureInPicture: false });
  });
});
