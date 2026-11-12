import { describe, expect, it } from 'vitest';
import { packageIdIssue, suggestedPackageId } from './package-id.ts';

describe('suggestedPackageId', () => {
  it.each([
    [{ slug: 'my-tv-app', android: { package: 'com.example.tv' } }, 'com.example.tv'],
    [{ slug: 'my-tv-app', android: { package: 'com.example.My_TV' } }, 'com.anonymous.my-tv-app'],
    [{ slug: 'My TV App!' }, 'com.anonymous.mytvapp'],
    [{ slug: '3d-viewer' }, 'com.anonymous.3d-viewer'],
    [{ slug: '..' }, 'com.anonymous'],
    [{}, 'com.anonymous.app'],
    ['not a config', 'com.anonymous.app'],
  ])('suggests from %j: %s', (expo, expected) => {
    expect(suggestedPackageId(expo)).toBe(expected);
  });
});

describe('packageIdIssue', () => {
  it('takes an id LG takes and says what is wrong with anything else', () => {
    expect(packageIdIssue('org.example.app')).toBeNull();
    expect(packageIdIssue('Example')).toContain('lowercase letters');
    expect(packageIdIssue('com.webos.mine')).toContain('LG keeps those');
  });
});
