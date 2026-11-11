import { afterEach, describe, expect, it } from 'vitest';
import { connectTv, registerTv } from './connect-tv.ts';
import { useTemporaryState } from './connected-tv.fixtures.ts';
import { fakeKeyServer, protectKey, rsaKeyPem, type FakeKeyServer } from './developer-key.fixtures.ts';
import { readDeviceKey } from './device-key.ts';
import { fakeTv, type FakeTv, type FakeTvOptions } from './fake-tv.fixtures.ts';
import { readKnownDevices } from './known-devices.ts';
import { installApp } from './install-app.ts';

const closers: { close(): Promise<void> }[] = [];

async function tvWithKeyServer(
  options?: FakeTvOptions,
): Promise<{ tv: FakeTv; keys: FakeKeyServer }> {
  const tv = await fakeTv(options);
  const keys = await fakeKeyServer(protectKey(tv.privateKey, 'A1b2C3'));
  closers.push(tv, keys);
  return { tv, keys };
}

useTemporaryState();

afterEach(async () => {
  await Promise.all(closers.splice(0).map((each) => each.close()));
});

describe('connectTv', () => {
  it('fetches the key, opens it, logs in and remembers the TV by its model', async () => {
    const { tv, keys } = await tvWithKeyServer();
    const address = { host: '127.0.0.1', port: tv.port };

    const device = await connectTv({
      host: '127.0.0.1',
      port: tv.port,
      keyServerPort: keys.port,
      passphrase: 'A1b2C3',
    });

    expect(device).toMatchObject({ name: 'OLED65C25LB', info: { sdkVersion: '11.2.0' } });
    expect(await readKnownDevices()).toEqual([{ name: 'OLED65C25LB', ...address }]);
    expect(readDeviceKey(address)).toMatch(/^-----BEGIN RSA PRIVATE KEY-----\n/);
    await expect(installApp(device, import.meta.filename)).resolves.toBeUndefined();
  });

  it('keeps nothing when the passphrase is wrong', async () => {
    const { tv, keys } = await tvWithKeyServer();

    const connecting = connectTv({
      host: '127.0.0.1',
      port: tv.port,
      keyServerPort: keys.port,
      passphrase: 'wrong1',
    });

    await expect(connecting).rejects.toThrow('The passphrase does not open the key');
    expect(await readKnownDevices()).toEqual([]);
    expect(tv.connections).toBe(0);
  });
});

describe('registerTv', () => {
  it('takes the name given, keeps it on a later connect, and refuses one another TV has', async () => {
    const first = await tvWithKeyServer();
    const second = await tvWithKeyServer();
    const firstAddress = { host: '127.0.0.1', port: first.tv.port };
    const secondAddress = { host: '127.0.0.1', port: second.tv.port };

    await registerTv({ address: firstAddress, privateKey: first.tv.privateKey, name: 'lounge' });
    const again = await registerTv({ address: firstAddress, privateKey: first.tv.privateKey });
    const namesake = registerTv({
      address: secondAddress,
      privateKey: second.tv.privateKey,
      name: 'lounge',
    });

    expect(again.name).toBe('lounge');
    await expect(namesake).rejects.toMatchObject({
      message: `The TV at 127.0.0.1:${first.tv.port} is already named lounge`,
      hint: 'Give this one another name with --name.',
    });
    expect(readDeviceKey(secondAddress)).toBeNull();
  });

  it('names a TV that does not tell its model after its address', async () => {
    const { tv } = await tvWithKeyServer({
      luna: { 'com.webos.service.tv.systemproperty/getSystemInfo': () => [{ returnValue: false }] },
    });

    const device = await registerTv({
      address: { host: '127.0.0.1', port: tv.port },
      privateKey: tv.privateKey,
    });

    expect(device.name).toBe('tv-127-0-0-1');
  });

  it('refuses a name --device could not tell from an address, and a key the TV refuses', async () => {
    const { tv } = await tvWithKeyServer();
    const address = { host: '127.0.0.1', port: tv.port };

    await expect(
      registerTv({ address, privateKey: tv.privateKey, name: '10.0.0.1' }),
    ).rejects.toThrow('cannot name a TV');
    await expect(registerTv({ address, privateKey: rsaKeyPem() })).rejects.toThrow(
      'served a key its own SSH server refuses',
    );
    expect(await readKnownDevices()).toEqual([]);
  });
});
