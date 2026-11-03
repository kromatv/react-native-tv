import { describe, expect, it } from 'vitest';
import { resolveTizenConfig } from './tizen-config.ts';

const expo = (tizen: Record<string, unknown>, extra: Record<string, unknown> = {}) => ({
  name: 'My App',
  version: '1.0.0',
  tizen: { package: 'org.example.app', ...tizen },
  ...extra,
});

describe('expo.tizen.voice', () => {
  it('turns voice on and asks for Samsung voice privileges', () => {
    const config = resolveTizenConfig(expo({ voice: true }));

    expect(config.features.voice).toEqual({ searchUrl: null });
    expect(config.privileges).toEqual([
      'http://tizen.org/privilege/internet',
      'http://developer.samsung.com/privilege/voicecontrol',
      'http://developer.samsung.com/privilege/microphone',
    ]);
  });

  it('keeps a search URL with known placeholders', () => {
    const config = resolveTizenConfig(
      expo({ voice: { searchUrl: 'myapp://search?q={query}&g={genre}' } }),
    );

    expect(config.features.voice).toEqual({ searchUrl: 'myapp://search?q={query}&g={genre}' });
  });

  it('refuses a search URL with an unknown placeholder', () => {
    expect(() =>
      resolveTizenConfig(expo({ voice: { searchUrl: 'myapp://search?q={name}' } })),
    ).toThrow('expo.tizen.voice.searchUrl: unknown placeholder');
  });

  it('refuses a search URL with no placeholder', () => {
    expect(() => resolveTizenConfig(expo({ voice: { searchUrl: 'myapp://search' } }))).toThrow(
      'expected at least one placeholder',
    );
  });
});

describe('expo.tizen.preview', () => {
  it('takes a public preview from an endpoint', () => {
    const config = resolveTizenConfig(
      expo({ preview: { endpoint: 'https://example.com/preview.json' } }),
    );

    expect(config.features.preview).toEqual({
      kind: 'public',
      endpoint: 'https://example.com/preview.json',
    });
    expect(config.privileges).toEqual(['http://tizen.org/privilege/internet']);
  });

  it('takes a personal preview and asks to launch its service app', () => {
    const config = resolveTizenConfig(expo({ preview: { refreshMinutes: 30 } }));

    expect(config.features.preview).toEqual({ kind: 'personal', refreshMinutes: 30 });
    expect(config.privileges).toContain('http://tizen.org/privilege/appmanager.launch');
  });

  it('refuses a refresh more often than Samsung allows', () => {
    expect(() => resolveTizenConfig(expo({ preview: { refreshMinutes: 5 } }))).toThrow(
      'expo.tizen.preview',
    );
  });
});

describe('expo.tizen.resolution', () => {
  it('defaults to 1080p', () => {
    expect(resolveTizenConfig(expo({})).features.resolution).toBe('1080p');
  });

  it('takes 4k', () => {
    expect(resolveTizenConfig(expo({ resolution: '4k' })).features.resolution).toBe('4k');
  });

  it('refuses another resolution', () => {
    expect(() => resolveTizenConfig(expo({ resolution: '8k' }))).toThrow('expo.tizen.resolution');
  });
});

describe('expo-video config plugin options', () => {
  it('reads supportsBackgroundPlayback and supportsPictureInPicture', () => {
    const config = resolveTizenConfig(
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
    const config = resolveTizenConfig(expo({}, { plugins: ['expo-video'] }));

    expect(config.features).toMatchObject({ backgroundMedia: false, pictureInPicture: false });
  });
});
