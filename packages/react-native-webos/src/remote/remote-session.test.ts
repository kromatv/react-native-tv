import { afterEach, describe, expect, it } from 'vitest';
import { fakeSsapTv, type FakeSsapTv, type FakeSsapTvOptions } from './fake-ssap-tv.fixtures.ts';
import { openRemote, type ClientKeyStore } from './remote-session.ts';

const tvs: FakeSsapTv[] = [];

async function tv(options?: FakeSsapTvOptions): Promise<FakeSsapTv> {
  const started = await fakeSsapTv(options);
  tvs.push(started);
  return started;
}

function memoryKeys(initial: Record<string, string> = {}): ClientKeyStore & {
  readonly saved: Record<string, string>;
} {
  const saved = { ...initial };
  return {
    saved,
    read: (tvId) => saved[tvId] ?? null,
    save: (tvId, key) => {
      saved[tvId] = key;
    },
  };
}

function options(started: FakeSsapTv, keys: ClientKeyStore) {
  return {
    host: '127.0.0.1',
    ports: { secure: 1, plain: started.port },
    timeoutMs: 2000,
    pairingTimeoutMs: 2000,
    keyIntervalMs: 0,
    keys,
  };
}

afterEach(async () => {
  await Promise.all(tvs.splice(0).map((started) => started.close()));
});

describe('openRemote', () => {
  it('pairs with a TV that does not know rn-webos and keeps the key under the TV id', async () => {
    const started = await tv();
    const keys = memoryKeys();
    let prompted = 0;

    const session = await openRemote({ ...options(started, keys), onPrompt: () => prompted++ });
    session.close();

    expect(prompted).toBe(1);
    expect(keys.saved).toEqual({ [started.uuid]: started.issuedKey });
    expect(started.frames.map((frame) => frame.type)).toEqual(['hello', 'register', 'request']);
  });

  it('registers with permissions only: no signed block for webOS 26 to refuse', async () => {
    const started = await tv();

    const session = await openRemote(options(started, memoryKeys()));
    session.close();

    expect(started.frames[1]).toEqual({
      id: 'register_0',
      type: 'register',
      payload: {
        forcePairing: false,
        pairingType: 'PROMPT',
        manifest: {
          manifestVersion: 1,
          appVersion: '1.1',
          permissions: expect.arrayContaining(['CONTROL_INPUT_TEXT', 'CONTROL_MOUSE_AND_KEYBOARD']),
        },
      },
    });
  });

  it('sends the stored key and is let in without a prompt', async () => {
    const started = await tv({ knownKeys: ['aaaabbbbccccdddd'], prompt: 'decline' });
    const keys = memoryKeys({ [started.uuid]: 'aaaabbbbccccdddd' });
    let prompted = 0;

    const session = await openRemote({ ...options(started, keys), onPrompt: () => prompted++ });
    session.close();

    expect(prompted).toBe(0);
    expect(started.frames[1]).toMatchObject({ payload: { 'client-key': 'aaaabbbbccccdddd' } });
    expect(keys.saved[started.uuid]).toBe('aaaabbbbccccdddd');
  });

  it('presses buttons on the pointer input socket, one frame each, in order', async () => {
    const started = await tv();

    const session = await openRemote(options(started, memoryKeys()));
    await session.press(['DOWN', 'ENTER']);
    session.close();

    expect(started.pointerFrames).toEqual([
      'type:button\nname:DOWN\n\n',
      'type:button\nname:ENTER\n\n',
    ]);
  });

  it("passes on the TV's words when its viewer declines", async () => {
    const started = await tv({ prompt: 'decline' });

    await expect(openRemote(options(started, memoryKeys()))).rejects.toMatchObject({
      message: 'The TV at 127.0.0.1 refused rn-webos: 403 User denied access',
      hint: expect.stringContaining('Accept the connection request'),
    });
  });

  it('asks for the prompt to be accepted when nobody answers it in time', async () => {
    const started = await tv({ prompt: 'ignore' });

    const opening = openRemote({ ...options(started, memoryKeys()), pairingTimeoutMs: 20 });

    await expect(opening).rejects.toThrow(
      'Nobody accepted rn-webos on the TV at 127.0.0.1 within 0.02 s',
    );
  });

  it('says the TV cannot be reached when nothing listens, or nothing answers', async () => {
    const silent = await tv({ answers: false });
    const gone = await tv();
    await gone.close();

    await expect(openRemote(options(gone, memoryKeys()))).rejects.toMatchObject({
      message: expect.stringContaining(
        'Cannot reach the remote control service of the TV at 127.0.0.1',
      ),
      hint: expect.stringContaining('LG Connect Apps'),
    });
    await expect(openRemote({ ...options(silent, memoryKeys()), timeoutMs: 30 })).rejects.toThrow(
      'no answer within 0.03 s',
    );
  });
});
