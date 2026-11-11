import { mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { describe, expect, it } from 'vitest';
import { parseProfiles, profilesPath, readProfiles } from './profiles-file.ts';

const PROFILES = `<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<profiles active="TV" version="3.1">
<profile name="TV">
<profileitem ca="" distributor="0" key="/home/me/SamsungCertificate/TV/author.p12" password="/home/me/SamsungCertificate/TV/author.pwd" rootca=""/>
<profileitem ca="" distributor="1" key="/home/me/SamsungCertificate/TV/distributor.p12" password="/home/me/SamsungCertificate/TV/distributor.pwd" rootca=""/>
<profileitem ca="" distributor="2" key="" password="" rootca=""/>
</profile>
<profile name="R&amp;D">
<profileitem ca="" distributor="0" key="/a.p12" password="inline" rootca=""/>
<profileitem ca="" distributor="1" key="/d1.p12" password="" rootca=""/>
<profileitem ca="" distributor="2" key="/d2.p12" password="" rootca=""/>
</profile>
</profiles>
`;

describe('parseProfiles', () => {
  it('reads the active profile and each profile certificates', () => {
    const file = parseProfiles(PROFILES);

    expect(file.active).toBe('TV');
    expect(file.profiles[0]).toEqual({
      name: 'TV',
      author: {
        path: '/home/me/SamsungCertificate/TV/author.p12',
        password: '/home/me/SamsungCertificate/TV/author.pwd',
      },
      distributors: [
        {
          path: '/home/me/SamsungCertificate/TV/distributor.p12',
          password: '/home/me/SamsungCertificate/TV/distributor.pwd',
        },
      ],
    });
  });

  it('decodes entities and keeps a second distributor', () => {
    const profile = parseProfiles(PROFILES).profiles[1];

    expect(profile?.name).toBe('R&D');
    expect(profile?.distributors.map((key) => key.path)).toEqual(['/d1.p12', '/d2.p12']);
  });

  it('finds nothing in a file without profiles', () => {
    expect(parseProfiles('<profiles version="3.1"></profiles>')).toEqual({
      active: null,
      profiles: [],
    });
  });
});

describe('profilesPath', () => {
  it('takes TIZEN_SDK_DATA_DIR first', () => {
    expect(profilesPath({ TIZEN_SDK_DATA_DIR: '/data' })).toBe('/data/profile/profiles.xml');
  });
});

describe('profilesPath and readProfiles', () => {
  it("takes the data directory Tizen Studio's sdk.info names", () => {
    const sdk = mkdtempSync(join(tmpdir(), 'rn-tizen-sdk-'));
    writeFileSync(
      join(sdk, 'sdk.info'),
      'TIZEN_SDK_INSTALLED_PATH=/x\nTIZEN_SDK_DATA_PATH=/data/tizen\n',
    );
    try {
      expect(profilesPath({ TIZEN_SDK_DIR: sdk })).toBe('/data/tizen/profile/profiles.xml');
    } finally {
      rmSync(sdk, { recursive: true, force: true });
    }
  });

  it('reads no profile where there is no file', () => {
    expect(readProfiles('/nonexistent/profiles.xml')).toEqual({ active: null, profiles: [] });
  });
});
