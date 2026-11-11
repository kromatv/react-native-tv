import { describe, expect, it } from 'vitest';
import { screenSize } from './screen-size.ts';

describe('screenSize', () => {
  it("keeps the instance's screen when no size was given", () => {
    expect(screenSize(undefined)).toBeUndefined();
  });

  it.each(['1920x1080', '1280x720'] as const)('takes %s, a plane a TV gives an app', (size) => {
    expect(screenSize(size)).toBe(size);
  });

  it.each(['3840x2160', '720p', ''])('refuses %s', (size) => {
    expect(() => screenSize(size)).toThrow('--screen takes 1920x1080 or 1280x720');
  });
});
