import { describe, expect, it } from 'vitest';
import { checkedAppId, closeApp, launchApp, uninstallApp } from './app-control.ts';
import { useFakeTvs, useTemporaryState } from './connected-tv.fixtures.ts';

useTemporaryState();
const connect = useFakeTvs();

describe('checkedAppId', () => {
  it('lets through what LG allows in an app id, and nothing a shell would read', () => {
    expect(checkedAppId('com.example.my-app+1')).toBe('com.example.my-app+1');
    expect(() => checkedAppId(`a'; reboot; '`)).toThrow('is not a webOS app id');
    expect(() => checkedAppId('Com.Example')).toThrow('is not a webOS app id');
  });
});

describe('launchApp', () => {
  it('launches, and reports what the TV said when it does not', async () => {
    const launched = await connect();
    const missing = await connect({
      luna: {
        'com.webos.applicationManager/launch': () => [
          { returnValue: false, errorCode: -101, errorText: 'Cannot find proper launchPoint' },
        ],
      },
    });

    await expect(launchApp(launched.device, 'com.example.myapp')).resolves.toBeUndefined();
    await expect(launchApp(missing.device, 'com.example.myapp')).rejects.toMatchObject({
      message: `127.0.0.1:${missing.tv.port} did not launch com.example.myapp`,
      hint: 'The TV said: Cannot find proper launchPoint',
    });
  });
});

describe('closeApp', () => {
  it('closes with dev/closeByAppId, and passes on why the TV did not', async () => {
    const running = await connect();
    const stopped = await connect({
      luna: {
        'com.webos.applicationManager/dev/closeByAppId': () => [
          { returnValue: false, errorCode: 2, errorText: 'com.example.myapp is not running' },
        ],
      },
    });

    await closeApp(running.device, 'com.example.myapp');

    expect(running.tv.commands).toEqual([
      `/usr/bin/luna-send-pub -n 1 luna://com.webos.applicationManager/dev/closeByAppId '{"id":"com.example.myapp","subscribe":false,"params":{"displayAffinity":0}}'`,
    ]);
    await expect(closeApp(stopped.device, 'com.example.myapp')).rejects.toMatchObject({
      hint: 'The TV said: com.example.myapp is not running',
    });
  });
});

describe('uninstallApp', () => {
  it('waits for the installer to say removed', async () => {
    const { tv, device } = await connect();

    await uninstallApp(device, 'com.example.myapp');

    expect(tv.commands).toEqual([
      `/usr/bin/luna-send-pub -i luna://com.webos.appInstallService/dev/remove '{"id":"com.example.myapp","subscribe":true}'`,
    ]);
  });

  it('reports an id the TV does not know, and a removal that failed', async () => {
    const unknown = await connect({
      luna: {
        'com.webos.appInstallService/dev/remove': () => [
          { returnValue: false, errorCode: -2, errorText: 'No such id', subscribed: false },
        ],
      },
    });
    const failing = await connect({
      luna: {
        'com.webos.appInstallService/dev/remove': () => [
          { returnValue: true, subscribed: true },
          { id: 'com.example.myapp', statusValue: 25, details: { state: 'remove failed' } },
        ],
      },
    });

    await expect(uninstallApp(unknown.device, 'com.example.myapp')).rejects.toMatchObject({
      message: `127.0.0.1:${unknown.tv.port} did not uninstall com.example.myapp`,
      hint: 'The TV said: No such id',
    });
    await expect(uninstallApp(failing.device, 'com.example.myapp')).rejects.toMatchObject({
      hint: 'The TV said: remove failed',
    });
  });
});
