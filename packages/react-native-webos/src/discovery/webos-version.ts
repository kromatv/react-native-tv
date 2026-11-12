// From 2022 LG names webOS after the year: releases 7.x to 11.x are webOS 22 to 26.
const FIRST_YEARLY_MAJOR = 7;
const LAST_KNOWN_MAJOR = 11;
const YEAR_OFFSET = 15;

/** A webOS release as LG markets it: `11.2.0` is `webOS 26 (11.2.0)`, `4.4.2` is `webOS 4.4.2`. */
export function webosVersionName(release: string): string {
  const major = Number(release.split('.')[0]);
  if (!Number.isInteger(major) || major < FIRST_YEARLY_MAJOR || major > LAST_KNOWN_MAJOR)
    return `webOS ${release}`;
  return `webOS ${major + YEAR_OFFSET} (${release})`;
}
