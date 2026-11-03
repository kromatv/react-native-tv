import { describe, expect, it } from 'vitest';
import { parseSecurityProfiles } from './security-profiles.ts';

const LISTING = [
  "Loaded in '/Users/me/tizen-studio-data/profile/profiles.xml'.",
  '[Profile Name]      [Active]  ',
  'KROMA               O         ',
  'My Old TV                     ',
  '',
].join('\n');

describe('parseSecurityProfiles', () => {
  it('reads every profile and which one is active', () => {
    expect(parseSecurityProfiles(LISTING)).toEqual([
      { name: 'KROMA', active: true },
      { name: 'My Old TV', active: false },
    ]);
  });

  it('finds no profile when the CLI printed no table', () => {
    expect(parseSecurityProfiles("Loaded in 'profiles.xml'.\n")).toEqual([]);
  });
});
