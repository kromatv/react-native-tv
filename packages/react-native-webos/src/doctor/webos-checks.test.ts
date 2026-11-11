import { describe, expect, it } from 'vitest';
import { useFakeTvs, useTemporaryState } from '../webos/connected-tv.fixtures.ts';
import { rememberDevice } from '../webos/known-devices.ts';
import { checkDevices } from './webos-checks.ts';

useTemporaryState();
const connect = useFakeTvs();

describe('checkDevices', () => {
  it('warns, with what to do on the TV, when none was ever connected', async () => {
    expect(await checkDevices()).toMatchObject({
      status: 'warn',
      detail: 'none connected yet',
      fix: expect.stringContaining('rn-webos connect <tv ip>'),
    });
  });

  it('names every TV that lets rn-webos in, with its model and webOS, and the ones that do not', async () => {
    const { tv } = await connect({ name: 'lounge' });
    await rememberDevice({ name: 'office', host: '127.0.0.1', port: 9 });

    expect(await checkDevices()).toEqual({
      title: 'TVs',
      status: 'ok',
      detail: `lounge at 127.0.0.1:${tv.port} (OLED65C25LB, webOS 26 (11.2.0)); not answering: office`,
    });
  });

  it('says Developer Mode is off when the SSH port of the only TV is closed', async () => {
    const { tv } = await connect({ name: 'lounge' });
    await tv.close();

    expect(await checkDevices()).toMatchObject({
      status: 'warn',
      detail: expect.stringContaining('Developer Mode is off, or its session ran out'),
      fix: expect.stringContaining('turn Dev Mode Status on'),
    });
  });

  it('says the key is missing, or no longer accepted, and how to fetch it again', async () => {
    const refusing = await connect({ name: 'lounge', acceptKey: false });
    await rememberDevice({ name: 'office', host: '127.0.0.1', port: 9 });

    const result = await checkDevices();

    expect(result.status).toBe('warn');
    expect(result.detail).toBe(
      `none of 2 answers: rn-webos has no key for the TV at 127.0.0.1:9; 127.0.0.1:${refusing.tv.port} refused the key rn-webos has for it`,
    );
    expect(result.fix).toContain('turn Key Server on, then run: rn-webos connect 127.0.0.1:9');
  });
});
