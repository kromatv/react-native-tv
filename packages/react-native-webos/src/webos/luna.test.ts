import { afterEach, describe, expect, it } from 'vitest';
import { z } from 'zod';
import { fakeTv, type FakeTv, type FakeTvOptions } from './fake-tv.fixtures.ts';
import { lunaCall, lunaCommand, lunaFollow, parseReply, shellQuote } from './luna.ts';
import { openSshSession, type SshSession } from './ssh-session.ts';

const tvs: FakeTv[] = [];
const sessions: SshSession[] = [];

async function session(options?: FakeTvOptions): Promise<SshSession> {
  const tv = await fakeTv(options);
  tvs.push(tv);
  const opened = await openSshSession({
    host: '127.0.0.1',
    port: tv.port,
    privateKey: tv.privateKey,
  });
  sessions.push(opened);
  return opened;
}

afterEach(async () => {
  for (const opened of sessions.splice(0)) opened.close();
  await Promise.all(tvs.splice(0).map((tv) => tv.close()));
});

describe('lunaCommand', () => {
  it('builds the line LG documents, the JSON quoted for the shell', () => {
    expect(lunaCommand('-n 1', 'luna://com.webos.applicationManager//launch', { id: 'a.b' })).toBe(
      `/usr/bin/luna-send-pub -n 1 luna://com.webos.applicationManager//launch '{"id":"a.b"}'`,
    );
    expect(shellQuote(`it's`)).toBe(`'it'\\''s'`);
  });
});

describe('parseReply', () => {
  it('reads one JSON object, and nothing else', () => {
    expect(parseReply('{"returnValue":true}')).toEqual({ returnValue: true });
    expect(parseReply('[1]')).toBeNull();
    expect(parseReply('luna-send: not found')).toBeNull();
    expect(parseReply(`{"a":"${'x'.repeat(600_000)}"}`)).toBeNull();
  });
});

describe('lunaCall', () => {
  const running = z.object({ running: z.array(z.object({ id: z.string() })) });

  it('returns the one reply, held to the schema', async () => {
    const opened = await session();

    const reply = await lunaCall(
      opened,
      'luna://com.webos.applicationManager/dev/running',
      { subscribe: false },
      running,
    );

    expect(reply.running).toEqual([{ id: 'com.example.myapp' }]);
  });

  it("throws the service's own text when it answers returnValue false", async () => {
    const opened = await session({
      luna: {
        'com.webos.applicationManager/dev/running': () => [
          { returnValue: false, errorCode: -1, errorText: 'Denied method call' },
        ],
      },
    });

    const calling = lunaCall(opened, 'luna://com.webos.applicationManager/dev/running', {}, running);

    await expect(calling).rejects.toMatchObject({ reason: 'Denied method call' });
  });

  it('refuses a reply of another shape, and a command that printed no reply', async () => {
    const opened = await session({
      luna: { 'com.webos.applicationManager/dev/running': () => [{ returnValue: true }] },
    });

    await expect(
      lunaCall(opened, 'luna://com.webos.applicationManager/dev/running', {}, running),
    ).rejects.toThrow('answered something unexpected');
    await expect(lunaCall(opened, 'luna://not a uri', {}, running)).rejects.toThrow(
      'did not answer: sh: /usr/bin/luna-send-pub: not found',
    );
  });
});

describe('lunaFollow', () => {
  const install = 'luna://com.webos.appInstallService/dev/install';

  it('reads replies until one settles the call', async () => {
    const opened = await session();
    const seen: unknown[] = [];

    const state = await lunaFollow(
      opened,
      install,
      { ipkUrl: '/media/developer/temp/a.ipk' },
      (reply) => {
        seen.push(reply.statusValue);
        return reply.statusValue === 30 ? 'done' : undefined;
      },
      5000,
    );

    expect(state).toBe('done');
    expect(seen).toEqual([undefined, 268, 35, 37, 32, 11, 13, 17, 27, 30]);
  });

  it('throws what the settle function throws, and the refusal of the service', async () => {
    const opened = await session({
      luna: {
        'com.webos.appInstallService/dev/remove': () => [
          { returnValue: false, errorCode: -2, errorText: 'No such id', subscribed: false },
        ],
      },
    });

    const stop = () => {
      throw new Error('stop');
    };

    await expect(lunaFollow(opened, install, {}, stop, 5000)).rejects.toThrow('stop');
    await expect(
      lunaFollow(opened, 'luna://com.webos.appInstallService/dev/remove', {}, () => undefined, 5000),
    ).rejects.toMatchObject({ reason: 'No such id' });
  });
});
