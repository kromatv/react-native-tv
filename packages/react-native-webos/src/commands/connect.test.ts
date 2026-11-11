import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { captureProcessOutput } from '../fakes.fixtures.ts';
import { useTemporaryState } from '../webos/connected-tv.fixtures.ts';
import { protectKey } from '../webos/developer-key.fixtures.ts';
import { readDeviceKey } from '../webos/device-key.ts';
import { fakeTv, type FakeTv } from '../webos/fake-tv.fixtures.ts';
import { fetchDeveloperKey } from '../webos/key-server.ts';
import { readKnownDevices } from '../webos/known-devices.ts';
import { connectCommand } from './connect.ts';

vi.mock('../webos/key-server.ts', async (importOriginal) => ({
  ...(await importOriginal<typeof import('../webos/key-server.ts')>()),
  fetchDeveloperKey: vi.fn(),
}));

const tvs: FakeTv[] = [];
let output: ReturnType<typeof captureProcessOutput>;
useTemporaryState();

async function tv(): Promise<FakeTv> {
  const started = await fakeTv();
  tvs.push(started);
  vi.mocked(fetchDeveloperKey).mockResolvedValue(protectKey(started.privateKey, 'A1b2C3'));
  return started;
}

beforeEach(() => {
  output = captureProcessOutput();
});

afterEach(async () => {
  vi.clearAllMocks();
  await Promise.all(tvs.splice(0).map((started) => started.close()));
});

describe('rn-webos connect', () => {
  it('fetches the key, opens it with --passphrase, logs in and remembers the TV', async () => {
    const started = await tv();
    const target = `127.0.0.1:${started.port}`;

    await connectCommand.run([target, '--passphrase', 'A1b2C3', '--name', 'lounge'], { cwd: '/' });

    expect(fetchDeveloperKey).toHaveBeenCalledWith('127.0.0.1');
    expect(await readKnownDevices()).toEqual([
      { name: 'lounge', host: '127.0.0.1', port: started.port },
    ]);
    expect(readDeviceKey({ host: '127.0.0.1', port: started.port })).not.toBeNull();
    expect(output.stdout()).toMatch(/Login\s+lounge, OLED65C25LB, webOS 26 \(11\.2\.0\)/);
    expect(output.stdout()).toContain('--device lounge');
  });

  it('says the passphrase is wrong, and connects nothing', async () => {
    const started = await tv();

    const connecting = connectCommand.run([`127.0.0.1:${started.port}`, '--passphrase', 'a1b2c3'], {
      cwd: '/',
    });

    await expect(connecting).rejects.toMatchObject({
      message: 'The passphrase does not open the key the TV served',
      hint: expect.stringContaining('six characters'),
    });
    expect(await readKnownDevices()).toEqual([]);
  });

  it('asks for --passphrase where it cannot ask on a terminal', async () => {
    await tv();

    await expect(connectCommand.run(['127.0.0.1'], { cwd: '/' })).rejects.toThrow(
      'connect needs --passphrase',
    );
  });

  it('asks for the address of one TV', async () => {
    await expect(connectCommand.run([], { cwd: '/' })).rejects.toThrow(
      "connect needs the TV's IP address",
    );
    await expect(connectCommand.run(['living-room'], { cwd: '/' })).rejects.toThrow(
      'living-room is not a TV address',
    );
    expect(fetchDeveloperKey).not.toHaveBeenCalled();
  });
});
