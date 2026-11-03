import { describe, expect, it } from 'vitest';
import type { TvFeatures } from '../config/tv-features.ts';
import { featureNotes } from './feature-notes.ts';

const none: TvFeatures = {
  voice: null,
  preview: null,
  resolution: '1080p',
  backgroundMedia: false,
  pictureInPicture: false,
};

describe('featureNotes', () => {
  it('says nothing when no feature is on', () => {
    expect(featureNotes(none, true)).toEqual([]);
  });

  it('says picture-in-picture is ignored in every build', () => {
    expect(featureNotes({ ...none, pictureInPicture: true }, false)).toEqual([
      'expo-video supportsPictureInPicture is ignored: Samsung TVs have no picture-in-picture for apps.',
    ]);
  });

  it('warns a release build about each feature Samsung does not publish for .NET apps', () => {
    const notes = featureNotes(
      {
        ...none,
        voice: { searchUrl: null },
        preview: { kind: 'personal', refreshMinutes: 10 },
        backgroundMedia: true,
      },
      true,
    );

    expect(notes).toHaveLength(3);
  });

  it('keeps a development build quiet about store review', () => {
    expect(
      featureNotes({ ...none, voice: { searchUrl: null }, backgroundMedia: true }, false),
    ).toEqual([]);
  });
});
