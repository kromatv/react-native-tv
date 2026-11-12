import { describe, expect, it } from 'vitest';
import { useFakeTvs, useTemporaryState } from './connected-tv.fixtures.ts';
import { listInstalledApps, listRunningApps } from './list-apps.ts';

useTemporaryState();
const connect = useFakeTvs();

describe('listInstalledApps', () => {
  it('lists the Developer Mode apps the TV shows, as LG lists them', async () => {
    const { tv, device } = await connect();

    const apps = await listInstalledApps(device);

    expect(apps).toEqual([
      { id: 'com.example.myapp', version: '1.0.0', title: 'My app', type: 'native' },
    ]);
    expect(tv.commands).toEqual([
      `/usr/bin/luna-send-pub -n 1 luna://com.webos.applicationManager/dev/listApps '{"subscribe":false}'`,
    ]);
  });
});

describe('listRunningApps', () => {
  it('lists the ids of the running apps', async () => {
    const { tv, device } = await connect();

    expect(await listRunningApps(device)).toEqual(['com.example.myapp']);
    expect(tv.commands).toEqual([
      `/usr/bin/luna-send-pub -n 1 luna://com.webos.applicationManager/dev/running '{"subscribe":false}'`,
    ]);
  });
});
