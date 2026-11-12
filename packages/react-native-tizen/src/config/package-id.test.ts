import { describe, expect, it } from 'vitest';
import { packageIdIssue, suggestedPackageId } from './package-id.ts';

describe('suggestedPackageId', () => {
  it.each([
    [{ slug: 'my-tv-app', android: { package: 'com.example.tv' } }, 'com.example.tv'],
    [{ slug: 'my-tv-app', android: { package: 'not a package' } }, 'com.anonymous.mytvapp'],
    [{ slug: '3d-viewer' }, 'com.anonymous.x3dviewer'],
    [{ slug: '..' }, 'com.anonymous'],
    [{}, 'com.anonymous.app'],
    ['not a config', 'com.anonymous.app'],
  ])('suggests from %j: %s', (expo, expected) => {
    expect(suggestedPackageId(expo)).toBe(expected);
  });
});

describe('packageIdIssue', () => {
  it('takes a reverse-DNS id and says what is wrong with anything else', () => {
    expect(packageIdIssue('org.example.app')).toBeNull();
    expect(packageIdIssue('example')).toContain('reverse-DNS');
    expect(packageIdIssue(`org.${'x'.repeat(60)}`)).not.toBeNull();
  });
});
