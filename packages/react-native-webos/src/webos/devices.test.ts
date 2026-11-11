import { describe, expect, it } from 'vitest';
import { FixableError } from '../errors.ts';
import { useFakeTvs, useTemporaryState } from './connected-tv.fixtures.ts';
import { listDevices, probeDevice, resolveDevice } from './devices.ts';
import { readKnownDevices, rememberDevice } from './known-devices.ts';
import { SshConnectError } from './ssh-session.ts';
import { connectFailure } from './tv-session.ts';

useTemporaryState();
const connect = useFakeTvs();

describe('probeDevice', () => {
  it('logs in and reads the model, the webOS release and the firmware', async () => {
    const { tv } = await connect({ name: 'lounge' });

    const device = await probeDevice({ name: 'lounge', host: '127.0.0.1', port: tv.port });

    expect(device).toEqual({
      address: { host: '127.0.0.1', port: tv.port },
      name: 'lounge',
      info: { modelName: 'OLED65C25LB', sdkVersion: '11.2.0', firmwareVersion: '43.21.74' },
    });
  });

  it('still answers for a TV that does not tell what it is', async () => {
    const { tv } = await connect({
      luna: {
        'com.webos.service.tv.systemproperty/getSystemInfo': () => [
          { returnValue: false, errorText: 'Denied method call' },
        ],
      },
    });

    const device = await probeDevice({ name: 'tv1', host: '127.0.0.1', port: tv.port });

    expect(device.info).toEqual({ modelName: '', sdkVersion: '', firmwareVersion: '' });
  });

  it('explains a TV that refuses the key, and one whose SSH port is closed', async () => {
    const refusing = await connect({ acceptKey: false });
    const gone = await connect();
    await gone.tv.close();

    await expect(
      probeDevice({ name: 'a', host: '127.0.0.1', port: refusing.tv.port }),
    ).rejects.toMatchObject({
      message: `127.0.0.1:${refusing.tv.port} refused the key rn-webos has for it`,
      hint: expect.stringContaining(`rn-webos connect 127.0.0.1:${refusing.tv.port}`),
    });
    await expect(
      probeDevice({ name: 'b', host: '127.0.0.1', port: gone.tv.port }),
    ).rejects.toThrow('Developer Mode is off');
  });
});

describe('connectFailure', () => {
  it('keeps an error that is not a connection failure', () => {
    const error = new Error('disk full');
    const address = { host: '192.168.1.50', port: 9922 };

    expect(connectFailure(address, error)).toBe(error);
    expect(connectFailure(address, 'text').message).toBe('text');
    expect(connectFailure(address, new SshConnectError('silent', 'quiet'))).toBeInstanceOf(
      FixableError,
    );
    expect(connectFailure(address, new SshConnectError('unreachable', 'x')).message).toBe(
      'No TV answers at 192.168.1.50',
    );
  });
});

describe('resolveDevice', () => {
  it('finds a connected TV by address or by name, and puts it first', async () => {
    const first = await connect({ name: 'lounge' });
    const second = await connect({ name: 'office' });

    const byName = await resolveDevice('lounge');
    const byAddress = await resolveDevice(`127.0.0.1:${second.tv.port}`);

    expect(byName.address.port).toBe(first.tv.port);
    expect(byAddress.name).toBe('office');
    expect((await readKnownDevices()).map((tv) => tv.name)).toEqual(['office', 'lounge']);
  });

  it('takes the only connected TV that answers when none is named', async () => {
    const answering = await connect({ name: 'lounge' });
    await rememberDevice({ name: 'gone', host: '127.0.0.1', port: 9 });

    expect((await resolveDevice(undefined)).address.port).toBe(answering.tv.port);
  });

  it('says how to connect a TV it does not know', async () => {
    await connect({ name: 'lounge' });

    await expect(resolveDevice('192.168.1.77')).rejects.toMatchObject({
      message: 'rn-webos is not connected to the TV at 192.168.1.77',
      hint: expect.stringContaining('rn-webos connect 192.168.1.77'),
    });
    await expect(resolveDevice('kitchen')).rejects.toMatchObject({
      message: 'No connected TV is named kitchen',
      hint: 'Pass its IP address, or one of: lounge',
    });
  });

  it('asks for a TV when none is connected, and which one when several answer', async () => {
    await expect(resolveDevice(undefined)).rejects.toThrow('No TV is connected');

    await connect({ name: 'lounge' });
    await connect({ name: 'office' });

    await expect(resolveDevice(undefined)).rejects.toMatchObject({
      message: '2 TVs are connected',
      hint: 'Pick one with --device: office, lounge',
    });
  });
});

describe('listDevices', () => {
  it('reports a remembered TV that does not answer as offline, with why', async () => {
    const { tv } = await connect({ name: 'lounge' });
    await tv.close();

    const [status] = await listDevices();

    expect(status).toMatchObject({ state: 'offline', known: { name: 'lounge' } });
    expect(status?.state === 'offline' && status.reason).toContain('Developer Mode is off');
  });
});
