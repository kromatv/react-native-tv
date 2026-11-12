import { describe, expect, it } from 'vitest';
import { PreviewValidationError, toSamsungPreview, type PreviewTile } from './preview-json.ts';

const tile = (overrides: Partial<PreviewTile> = {}): PreviewTile => ({
  image: 'https://example.com/poster.jpg',
  url: 'myapp://item/1',
  ...overrides,
});

describe('toSamsungPreview', () => {
  it('writes each tile as Samsung tile JSON with its url in action_data', () => {
    const preview = toSamsungPreview([
      {
        title: 'Continue watching',
        tiles: [
          tile({ title: 'Blade Runner', subtitle: '1h 12m left', ratio: '16:9', playable: true }),
        ],
      },
    ]);

    expect(preview).toEqual({
      sections: [
        {
          title: 'Continue watching',
          position: 0,
          tiles: [
            {
              title: 'Blade Runner',
              subtitle: '1h 12m left',
              image_url: 'https://example.com/poster.jpg',
              image_ratio: '16by9',
              action_data: '{"url":"myapp://item/1"}',
              is_playable: true,
              position: 0,
            },
          ],
        },
      ],
    });
  });

  it('defaults a tile to Samsung square ratio and no Play icon', () => {
    const [section] = toSamsungPreview([{ tiles: [tile()] }]).sections;

    expect(section?.tiles[0]).toMatchObject({ image_ratio: '1by1', is_playable: false });
  });

  it('writes display dates in UNIX seconds', () => {
    const [section] = toSamsungPreview([
      {
        tiles: [
          tile({
            showFrom: new Date('2026-11-03T00:00:00Z'),
            showUntil: new Date('2026-11-10T00:00:00Z'),
          }),
        ],
      },
    ]).sections;

    expect(section?.tiles[0]).toMatchObject({
      display_from: 1793664000,
      display_until: 1794268800,
    });
  });

  it('numbers sections and tiles in the order given', () => {
    const preview = toSamsungPreview([
      { tiles: [tile({ url: 'myapp://a' }), tile({ url: 'myapp://b' })] },
      { tiles: [tile({ url: 'myapp://c' })] },
    ]);

    expect(preview.sections.map((section) => section.position)).toEqual([0, 1]);
    expect(preview.sections[0]?.tiles.map((entry) => entry.position)).toEqual([0, 1]);
  });

  it('refuses a preview with no section', () => {
    expect(() => toSamsungPreview([])).toThrow('a preview needs at least one section');
  });

  it('refuses a section with no tile', () => {
    expect(() => toSamsungPreview([{ tiles: [] }])).toThrow('a section needs at least one tile');
  });

  it('refuses more than forty tiles across sections', () => {
    const tiles = Array.from({ length: 21 }, (_, index) => tile({ url: `myapp://item/${index}` }));

    expect(() =>
      toSamsungPreview([
        { tiles },
        { tiles: tiles.map((entry) => ({ ...entry, url: `${entry.url}b` })) },
      ]),
    ).toThrow('a preview holds at most 40 tiles');
  });

  it('refuses two tiles with the same url', () => {
    expect(() => toSamsungPreview([{ tiles: [tile(), tile()] }])).toThrow(
      'every tile needs its own url',
    );
  });

  it('refuses an image that is not http or https', () => {
    expect(() => toSamsungPreview([{ tiles: [tile({ image: 'file:///poster.jpg' })] }])).toThrow(
      PreviewValidationError,
    );
  });

  it('refuses a tile shown until before it is shown from', () => {
    expect(() =>
      toSamsungPreview([
        { tiles: [tile({ showFrom: new Date(2_000_000), showUntil: new Date(1_000_000) })] },
      ]),
    ).toThrow('showFrom must come before showUntil');
  });

  it('names the tile that broke a rule', () => {
    expect(() => toSamsungPreview([{ tiles: [tile({ url: 'not a url' })] }])).toThrow(
      /^0\.tiles\.0\.url: /,
    );
  });
});
