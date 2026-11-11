import { describe, expect, it } from 'vitest';
import { formatDuration, formatPath, formatSize } from './format.ts';

describe('formatDuration', () => {
  it('keeps a tenth of a second under ten seconds', () => {
    expect(formatDuration(420)).toBe('0.4s');
  });

  it('rounds to whole seconds under a minute', () => {
    expect(formatDuration(12_400)).toBe('12s');
  });

  it('splits minutes from seconds', () => {
    expect(formatDuration(125_000)).toBe('2m 05s');
  });
});

describe('formatSize', () => {
  it('prints bytes as they are', () => {
    expect(formatSize(912)).toBe('912 B');
  });

  it('prints larger sizes in decimal units with one decimal', () => {
    expect(formatSize(4_105_506)).toBe('4.1 MB');
  });
});

describe('formatPath', () => {
  it('prints a path inside the working directory relative to it', () => {
    expect(formatPath('/app/tizen/build/app.tpk', '/app')).toBe('tizen/build/app.tpk');
  });

  it('prints a path outside the working directory as it is', () => {
    expect(formatPath('/tmp/bundle/index.tizen.hbc', '/app/example')).toBe(
      '/tmp/bundle/index.tizen.hbc',
    );
  });
});
