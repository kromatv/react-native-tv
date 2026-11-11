import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { captureProcessOutput } from '../fakes.fixtures.ts';
import { fakeSsapTv, type FakeSsapTv } from '../remote/fake-ssap-tv.fixtures.ts';
import { pressRemoteKeys } from '../remote/press-keys.ts';
import { useTemporaryState } from '../webos/connected-tv.fixtures.ts';
import { rememberDevice } from '../webos/known-devices.ts';
import { keyCommand } from './key.ts';

import { vi } from 'vitest';

vi.mock('../remote/press-keys.ts', async (importOriginal) => {
  const original = await importOriginal<typeof import('../remote/press-keys.ts')>();
  return { ...original, pressRemoteKeys: vi.fn(original.pressRemoteKeys) };
});

const tvs: FakeSsapTv[] = [];
let output: ReturnType<typeof captureProcessOutput>;
useTemporaryState();

/** Sends the command's keys to a fake TV on a free port instead of ports 3001 and 3000. */
async function tv(): Promise<FakeSsapTv> {
  const started = await fakeSsapTv();
  tvs.push(started);
  const { pressRemoteKeys: press } =
    await vi.importActual<typeof import('../remote/press-keys.ts')>('../remote/press-keys.ts');
  vi.mocked(pressRemoteKeys).mockImplementation((host, keys, options) =>
    press(host, keys, {
      ...options,
      ports: { secure: 1, plain: started.port },
      keyIntervalMs: 0,
    }),
  );
  return started;
}

beforeEach(() => {
  output = captureProcessOutput();
});

afterEach(async () => {
  vi.clearAllMocks();
  await Promise.all(tvs.splice(0).map((started) => started.close()));
});

describe('rn-webos key', () => {
  it('pairs with a TV it never paired with, then presses the keys in order', async () => {
    const started = await tv();

    await keyCommand.run(['Down', 'Enter', '--device', '127.0.0.1'], { cwd: '/' });

    expect(started.pointerFrames).toEqual([
      'type:button\nname:DOWN\n\n',
      'type:button\nname:ENTER\n\n',
    ]);
    expect(output.stdout()).toContain(
      'rn-webos has not paired with the TV at 127.0.0.1 yet: accept the connection request it shows.',
    );
    expect(output.stdout()).toMatch(/Keys\s+Down Enter/);
  });

  it('uses the key the TV issued the next time, without a prompt', async () => {
    const started = await tv();

    await keyCommand.run(['Back', '-d', '127.0.0.1'], { cwd: '/' });
    await keyCommand.run(['1', '-d', '127.0.0.1'], { cwd: '/' });

    expect(started.frames.filter((frame) => frame.type === 'register')[1]).toMatchObject({
      payload: { 'client-key': started.issuedKey },
    });
    expect(output.stdout().match(/has not paired/g)).toHaveLength(1);
    expect(started.pointerFrames).toEqual([
      'type:button\nname:BACK\n\n',
      'type:button\nname:1\n\n',
    ]);
  });

  it('takes the TV connected last without --device, or one by its name', async () => {
    await tv();
    await rememberDevice({ name: 'office', host: '10.9.9.9', port: 9922 });
    await rememberDevice({ name: 'lounge', host: '127.0.0.1', port: 9922 });

    await keyCommand.run(['Up'], { cwd: '/' });
    await keyCommand.run(['Up', '--device', 'lounge'], { cwd: '/' });

    expect(vi.mocked(pressRemoteKeys).mock.calls.map(([host]) => host)).toEqual([
      '127.0.0.1',
      '127.0.0.1',
    ]);
  });

  it('asks for the keys to press, and for the TV when none was ever connected', async () => {
    await expect(keyCommand.run(['--device', '127.0.0.1'], { cwd: '/' })).rejects.toThrow(
      'key needs the keys to press',
    );
    await expect(keyCommand.run(['Down'], { cwd: '/' })).rejects.toThrow('key needs --device');
    await expect(keyCommand.run(['Down', '-d', 'kitchen'], { cwd: '/' })).rejects.toThrow(
      "--device must be the TV's IP address or the name it was connected under, got kitchen",
    );
  });

  it('refuses a key it does not know before it connects', async () => {
    await expect(
      keyCommand.run(['Sideways', '--device', '127.0.0.1'], { cwd: '/' }),
    ).rejects.toThrow('Unknown key Sideways');
    expect(pressRemoteKeys).not.toHaveBeenCalled();
  });
});
