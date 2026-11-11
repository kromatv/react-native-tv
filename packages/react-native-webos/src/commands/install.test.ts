import { writeFileSync } from 'node:fs';
import { join } from 'node:path';
import { beforeEach, describe, expect, it } from 'vitest';
import { captureProcessOutput } from '../fakes.fixtures.ts';
import { useFakeTvs, useTemporaryState } from '../webos/connected-tv.fixtures.ts';
import { failedInstall } from '../webos/fake-tv.fixtures.ts';
import { installCommand } from './install.ts';
import { uninstallCommand } from './uninstall.ts';

const LAUNCH = `/usr/bin/luna-send-pub -n 1 luna://com.webos.applicationManager//launch '{"id":"com.example.myapp","subscribe":false,"params":{"displayAffinity":0}}'`;

let output: ReturnType<typeof captureProcessOutput>;
const state = useTemporaryState();
const connect = useFakeTvs();

function ipk(name = 'app.ipk'): string {
  writeFileSync(join(state(), name), 'an ipk');
  return name;
}

beforeEach(() => {
  output = captureProcessOutput();
});

describe('rn-webos install', () => {
  it('installs an .ipk on the only connected TV', async () => {
    const { tv } = await connect({ name: 'lounge' });

    await installCommand.run([ipk()], { cwd: state() });

    expect(tv.installed.map(String)).toEqual(['an ipk']);
    expect(tv.commands).not.toContain(LAUNCH);
    expect(output.stdout()).toMatch(/TV\s+lounge/);
  });

  it('launches the app the TV read from the package with --launch, on the same login', async () => {
    const { tv } = await connect({ name: 'lounge' });

    await installCommand.run([ipk(), '--launch', '--device', 'lounge'], { cwd: state() });

    expect(tv.commands.at(-1)).toBe(LAUNCH);
    expect(tv.connections).toBe(2);
    expect(output.stdout()).toMatch(/Launch\s+com\.example\.myapp/);
  });

  it('reads the app id off the file name when the TV names no package', async () => {
    const { tv } = await connect({
      luna: {
        'com.webos.appInstallService/dev/install': () => [
          { returnValue: true, subscribed: true },
          { id: 'com.ares.defaultName', statusValue: 30, details: { state: 'installed' } },
        ],
      },
    });

    await installCommand.run([ipk('com.example.myapp_1.0.0_arm.ipk'), '--launch'], {
      cwd: state(),
    });
    const unnamed = installCommand.run([ipk(), '--launch'], { cwd: state() });

    expect(tv.commands).toContain(LAUNCH);
    await expect(unnamed).rejects.toThrow('the TV did not say which app it holds');
  });

  it("stops with the TV's reason when it refuses the package", async () => {
    await connect({
      luna: { 'com.webos.appInstallService/dev/install': failedInstall('FAILED_IPKG_INSTALL') },
    });

    await expect(installCommand.run([ipk(), '--launch'], { cwd: state() })).rejects.toThrow(
      'did not install the package: FAILED_IPKG_INSTALL',
    );
  });

  it('asks for one package', async () => {
    await expect(installCommand.run([], { cwd: state() })).rejects.toThrow(
      'install needs one package',
    );
  });
});

describe('rn-webos uninstall', () => {
  it('removes the app from the TV named', async () => {
    const { tv } = await connect({ name: 'lounge' });

    await uninstallCommand.run(['com.example.myapp', '-d', `127.0.0.1:${tv.port}`], {
      cwd: state(),
    });

    expect(tv.commands).toEqual([
      `/usr/bin/luna-send-pub -n 1 luna://com.webos.service.tv.systemproperty//getSystemInfo '{"keys":["modelName","sdkVersion","firmwareVersion","boardType","otaId"],"subscribe":false}'`,
      `/usr/bin/luna-send-pub -i luna://com.webos.appInstallService/dev/remove '{"id":"com.example.myapp","subscribe":true}'`,
    ]);
  });

  it('asks for one app id, and refuses one that is not an id', async () => {
    await connect();

    await expect(uninstallCommand.run([], { cwd: state() })).rejects.toThrow(
      'uninstall needs one app id',
    );
    await expect(uninstallCommand.run(['My_App'], { cwd: state() })).rejects.toThrow(
      'is not a webOS app id',
    );
  });
});
