import { describe, expect, it } from 'vitest';
import { distTag } from './dist-tag.ts';

describe('distTag', () => {
  it('puts a stable version on latest', () => {
    expect(distTag('0.1.0', ['0.1.0-alpha.3'])).toBe('latest');
    expect(distTag('1.2.0', ['1.1.0'])).toBe('latest');
  });

  it('puts a prerelease on latest while npm has no stable version', () => {
    expect(distTag('0.1.0-alpha.1', [])).toBe('latest');
    expect(distTag('0.1.0-alpha.1', ['0.0.0-stage', '0.1.0-alpha.0'])).toBe('latest');
  });

  it('puts a prerelease on next once a stable version is out', () => {
    expect(distTag('0.2.0-beta.0', ['0.1.0-alpha.3', '0.1.0'])).toBe('next');
  });
});
