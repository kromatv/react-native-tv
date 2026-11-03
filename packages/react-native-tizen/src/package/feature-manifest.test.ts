import { describe, expect, it } from 'vitest';
import type { TvFeatures } from '../config/tv-features.ts';
import { applicationExtras, projectReferences } from './feature-manifest.ts';

const none: TvFeatures = {
  voice: null,
  preview: null,
  resolution: '1080p',
  backgroundMedia: false,
  pictureInPicture: false,
};

describe('applicationExtras', () => {
  it('adds nothing when no feature is on', () => {
    expect(applicationExtras(none)).toBe('');
  });

  it('asks the window manager for a 4K UI plane', () => {
    expect(applicationExtras({ ...none, resolution: '4k' })).toBe(
      '\n    <metadata key="http://tizen.org/metadata/app_ui_type/base_screen_resolution" value="4k_only" />',
    );
  });

  it('declares the media background category for background playback', () => {
    expect(applicationExtras({ ...none, backgroundMedia: true })).toBe(
      '\n    <background-category value="media" />',
    );
  });

  it('points a public preview at its endpoint, escaped', () => {
    expect(
      applicationExtras({
        ...none,
        preview: { kind: 'public', endpoint: 'https://example.com/p.json?a=1&b=2' },
      }),
    ).toBe(
      '\n    <metadata key="http://samsung.com/tv/metadata/use.preview" value="endpoint_URL=https://example.com/p.json?a=1&amp;b=2" />',
    );
  });

  it('declares a personal preview after its app control, in schema order', () => {
    const extras = applicationExtras({
      ...none,
      preview: { kind: 'personal', refreshMinutes: 10 },
      resolution: '4k',
      backgroundMedia: true,
    });

    expect(extras.indexOf('<app-control>')).toBeLessThan(
      extras.indexOf('use.preview" value="bg_service"'),
    );
    expect(extras.indexOf('bg_service')).toBeLessThan(extras.indexOf('<background-category'));
  });
});

describe('projectReferences', () => {
  it('packs the preview service app with a personal preview only', () => {
    expect(projectReferences(none)).toBe('');
    expect(projectReferences({ ...none, preview: { kind: 'personal', refreshMinutes: 10 } })).toBe(
      '\n    <ProjectReference Include="PreviewService/PreviewService.csproj" />',
    );
  });
});
