import { mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, describe, expect, it } from 'vitest';
import { useFakeTvs, useTemporaryState } from './connected-tv.fixtures.ts';
import { failedInstall } from './fake-tv.fixtures.ts';
import { installApp, installOn } from './install-app.ts';
import { launchOn } from './app-control.ts';
import { onDevice } from './tv-session.ts';

const dirs: string[] = [];
const REMOTE = '/media/developer/temp/rn-webos-6d3a1128d5.ipk';

function ipk(content = 'an ipk'): string {
  const dir = mkdtempSync(join(tmpdir(), 'rn-webos-install-'));
  dirs.push(dir);
  const path = join(dir, 'com.example.myapp_1.0.0_arm.ipk');
  writeFileSync(path, content);
  return path;
}

useTemporaryState();
const connect = useFakeTvs();

afterEach(() => {
  for (const dir of dirs.splice(0)) rmSync(dir, { recursive: true, force: true });
});

describe('installApp', () => {
  it('pushes, checks, installs and removes the temporary file, in the order LG documents', async () => {
    const { tv, device } = await connect();
    const progressed: string[] = [];

    await installApp(device, ipk(), (detail) => progressed.push(detail));

    expect(tv.commands).toEqual([
      '/usr/bin/test -d /media/developer/temp || /bin/mkdir -p /media/developer/temp',
      `/usr/bin/md5sum ${REMOTE}`,
      `/usr/bin/luna-send-pub -i luna://com.webos.appInstallService/dev/install '{"id":"com.ares.defaultName","ipkUrl":"${REMOTE}","subscribe":true}'`,
      `/bin/rm -f ${REMOTE}`,
    ]);
    expect(tv.installed.map(String)).toEqual(['an ipk']);
    expect(tv.files.size).toBe(0);
    expect(progressed).toEqual([
      'sending 100%',
      'ipk verifying',
      'ipk parsing',
      'ipk parsing',
      'app closing',
      'installing',
      'installing : start',
      'installing : installing',
      'installing : service installing',
      'installed',
    ]);
  });

  it('installs through cat on a TV without sftp-server', async () => {
    const { tv, device } = await connect({ sftp: false });

    await installApp(device, ipk());

    expect(tv.transfers).toEqual(['cat']);
    expect(tv.installed.map(String)).toEqual(['an ipk']);
  });

  it('installs on a TV that cannot check the package, and refuses one that arrived damaged', async () => {
    const unchecked = await connect({ md5sum: 'missing' });
    const damaged = await connect({ md5sum: 'wrong' });

    await installApp(unchecked.device, ipk());

    expect(unchecked.tv.installed).toHaveLength(1);
    await expect(installApp(damaged.device, ipk())).rejects.toThrow(
      'did not install the package: the package arrived damaged',
    );
    expect(damaged.tv.installed).toHaveLength(0);
    expect(damaged.tv.files.size).toBe(0);
  });

  it("fails with the TV's reason, and still removes the temporary file", async () => {
    const { tv, device } = await connect({
      luna: {
        'com.webos.appInstallService/dev/install': failedInstall('Failed to extract package'),
      },
    });

    const installing = installApp(device, ipk());

    await expect(installing).rejects.toMatchObject({
      message: `127.0.0.1:${tv.port} did not install the package: Failed to extract package`,
      hint: expect.stringContaining('Developer Mode'),
    });
    expect(tv.commands.at(-1)).toBe(`/bin/rm -f ${REMOTE}`);
  });

  it('points at the storage when the TV has no room', async () => {
    const { device } = await connect({
      luna: {
        'com.webos.appInstallService/dev/install': failedInstall(
          'There is no available space to install App',
        ),
      },
    });

    await expect(installApp(device, ipk())).rejects.toMatchObject({
      hint: expect.stringContaining('rn-webos uninstall'),
    });
  });

  it('passes on a refusal of the install service itself', async () => {
    const { device } = await connect({
      luna: {
        'com.webos.appInstallService/dev/install': () => [
          {
            returnValue: false,
            errorCode: -1,
            errorText: 'Unknown method "install" for category "/dev"',
          },
        ],
      },
    });

    await expect(installApp(device, ipk())).rejects.toThrow(
      'did not install the package: Unknown method "install" for category "/dev"',
    );
  });

  it('says what to do when rn-webos has no key for the TV', async () => {
    const device = { address: { host: '127.0.0.1', port: 9 } };

    await expect(installApp(device, ipk())).rejects.toMatchObject({
      message: 'rn-webos has no key for the TV at 127.0.0.1:9',
      hint: expect.stringContaining('rn-webos connect 127.0.0.1:9'),
    });
  });
});

describe('install, then launch', () => {
  it('chains both on one login', async () => {
    const { tv, device } = await connect();

    await onDevice(device, async (session) => {
      await installOn(session, device, ipk());
      await launchOn(session, device, 'com.example.myapp');
    });

    expect(tv.connections).toBe(1);
    expect(tv.commands.at(-1)).toBe(
      `/usr/bin/luna-send-pub -n 1 luna://com.webos.applicationManager//launch '{"id":"com.example.myapp","subscribe":false,"params":{"displayAffinity":0}}'`,
    );
  });
});
