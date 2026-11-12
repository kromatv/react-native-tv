import type { TvFeatures } from '../config/tv-features.ts';

/**
 * What `rn-tizen package` says about the app's TV features: what Tizen
 * ignores, and what a store build should know before review.
 */
export function featureNotes(features: TvFeatures, release: boolean): readonly string[] {
  const notes: string[] = [];
  if (features.pictureInPicture) {
    notes.push(
      'expo-video supportsPictureInPicture is ignored: Samsung TVs have no picture-in-picture for apps.',
    );
  }
  if (!release) return notes;
  if (features.backgroundMedia) {
    notes.push(
      'Background audio: Samsung asks that a hidden app pause; check with your content manager before review.',
    );
  }
  if (features.voice !== null) {
    notes.push(
      "Voice uses Samsung's unpublished voice service protocol; check with your content manager before review.",
    );
  }
  if (features.preview?.kind === 'personal') {
    notes.push(
      'The personal Smart Hub preview uses unpublished TV APIs; check with your content manager before review.',
    );
  }
  return notes;
}
