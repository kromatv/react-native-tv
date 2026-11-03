import { describe, expect, it } from 'vitest';
import { openRemote, remoteUrl } from './remote-session.ts';
import type { OpenTvConnection, TvConnectionEvents } from './tv-socket.ts';

interface FakeTv {
  readonly open: OpenTvConnection;
  readonly sent: string[];
  readonly urls: string[];
  answer(message: object): void;
  drop(reason: string): void;
  closed(): boolean;
}

function fakeTv(): FakeTv {
  const sent: string[] = [];
  const urls: string[] = [];
  let events: TvConnectionEvents | null = null;
  let isClosed = false;
  return {
    sent,
    urls,
    open: (url, handlers) => {
      urls.push(url);
      events = handlers;
      return {
        send: (text) => sent.push(text),
        close: () => {
          isClosed = true;
        },
        bufferedAmount: () => 0,
      };
    },
    answer: (message) => events?.onMessage(JSON.stringify(message)),
    drop: (reason) => events?.onClosed(reason),
    closed: () => isClosed,
  };
}

const options = { host: '192.168.1.50', timeoutMs: 1000, keyIntervalMs: 0 };

describe('remoteUrl', () => {
  it('names the client rn-tizen in base64 and adds the token when there is one', () => {
    expect(remoteUrl('192.168.1.50', null)).toBe(
      'wss://192.168.1.50:8002/api/v2/channels/samsung.remote.control?name=cm4tdGl6ZW4=',
    );
    expect(remoteUrl('192.168.1.50', '12345678')).toMatch(/&token=12345678$/);
  });
});

describe('openRemote', () => {
  it('waits past other events for ms.channel.connect and keeps the token it carries', async () => {
    const tv = fakeTv();

    const opening = openRemote({ ...options, token: null, open: tv.open });
    tv.answer({ event: 'ms.remote.touchEnable' });
    tv.answer({ event: 'ms.channel.connect', data: { token: 87654321 } });
    const session = await opening;

    expect(session.token).toBe('87654321');
  });

  it('clicks each key in order', async () => {
    const tv = fakeTv();

    const opening = openRemote({ ...options, token: '1', open: tv.open });
    tv.answer({ event: 'ms.channel.connect', data: {} });
    await (await opening).press(['KEY_DOWN', 'KEY_ENTER']);

    expect(tv.sent.map((text) => JSON.parse(text))).toEqual(
      ['KEY_DOWN', 'KEY_ENTER'].map((code) => ({
        method: 'ms.remote.control',
        params: { Cmd: 'Click', DataOfCmd: code, Option: 'false', TypeOfRemote: 'SendRemoteKey' },
      })),
    );
  });

  it('ignores a token that is not one', async () => {
    const tv = fakeTv();

    const opening = openRemote({ ...options, token: null, open: tv.open });
    tv.answer({ event: 'ms.channel.connect', data: { token: '../../etc' } });

    expect((await opening).token).toBeNull();
  });

  it('says where to allow rn-tizen when the TV refuses it, and closes', async () => {
    const tv = fakeTv();

    const opening = openRemote({ ...options, token: '1', open: tv.open });
    tv.answer({ event: 'ms.channel.unauthorized' });

    await expect(opening).rejects.toMatchObject({
      message: 'The TV at 192.168.1.50 refused rn-tizen',
      hint: expect.stringContaining('Device Connection Manager'),
    });
    expect(tv.closed()).toBe(true);
  });

  it('fails when the connection drops before the TV accepts it', async () => {
    const tv = fakeTv();

    const opening = openRemote({ ...options, token: '1', open: tv.open });
    tv.drop('the connection failed');

    await expect(opening).rejects.toThrow(
      'Cannot reach the remote API of the TV at 192.168.1.50:8002 (the connection failed)',
    );
  });

  it('asks for Allow when nobody answered the pairing prompt in time', async () => {
    const tv = fakeTv();

    const opening = openRemote({ ...options, timeoutMs: 10, token: null, open: tv.open });

    await expect(opening).rejects.toThrow('Nobody allowed rn-tizen on the TV within 0.01 s');
  });
});
