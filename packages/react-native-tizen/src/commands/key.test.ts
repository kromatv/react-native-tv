import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { captureProcessOutput } from '../fakes.fixtures.ts';
import { openRemote, type RemoteSession } from '../remote/remote-session.ts';
import { readToken, saveToken } from '../remote/remote-token.ts';
import { keyCommand } from './key.ts';

vi.mock('../remote/remote-session.ts', () => ({ openRemote: vi.fn() }));
vi.mock('../remote/remote-token.ts', () => ({ readToken: vi.fn(), saveToken: vi.fn() }));

const HOST = '192.168.1.50';

function session(token: string | null): RemoteSession {
  return { token, press: vi.fn(async () => undefined), close: vi.fn() };
}

let output: ReturnType<typeof captureProcessOutput>;

beforeEach(() => {
  output = captureProcessOutput();
});

afterEach(() => {
  vi.clearAllMocks();
});

describe('rn-tizen key', () => {
  it('presses the keys with the saved token, then closes the connection', async () => {
    vi.mocked(readToken).mockReturnValue('12345');
    const remote = session('12345');
    vi.mocked(openRemote).mockResolvedValue(remote);

    await keyCommand.run(['Down', 'Enter', '--device', HOST], { cwd: '/' });

    expect(openRemote).toHaveBeenCalledWith(
      expect.objectContaining({ host: HOST, token: '12345' }),
    );
    expect(remote.press).toHaveBeenCalledWith(['KEY_DOWN', 'KEY_ENTER']);
    expect(remote.close).toHaveBeenCalled();
    expect(saveToken).not.toHaveBeenCalled();
  });

  it('pairs first with a TV it never paired with, and keeps the token it issued', async () => {
    vi.mocked(readToken).mockReturnValue(null);
    const remote = session('67890');
    vi.mocked(openRemote).mockResolvedValue(remote);

    await keyCommand.run(['Back', '-d', HOST], { cwd: '/' });

    expect(openRemote).toHaveBeenCalledTimes(1);
    expect(openRemote).toHaveBeenCalledWith(expect.objectContaining({ token: null }));
    expect(saveToken).toHaveBeenCalledWith(HOST, '67890');
    expect(output.stdout()).toContain('choose Allow when the TV asks');
  });

  it('keeps a new token the TV issues on a later connection', async () => {
    vi.mocked(readToken).mockReturnValue('old');
    vi.mocked(openRemote).mockResolvedValue(session('new'));

    await keyCommand.run(['1', '--device', HOST], { cwd: '/' });

    expect(saveToken).toHaveBeenCalledWith(HOST, 'new');
  });

  it('closes the connection when a press fails', async () => {
    vi.mocked(readToken).mockReturnValue('12345');
    const remote = session('12345');
    vi.mocked(remote.press).mockRejectedValue(new Error('closed'));
    vi.mocked(openRemote).mockResolvedValue(remote);

    await expect(keyCommand.run(['Up', '--device', HOST], { cwd: '/' })).rejects.toThrow('closed');
    expect(remote.close).toHaveBeenCalled();
  });

  it('asks for the keys to press', async () => {
    await expect(keyCommand.run(['--device', HOST], { cwd: '/' })).rejects.toThrow(
      'key needs the keys to press',
    );
  });

  it('asks for the TV', async () => {
    await expect(keyCommand.run(['Down'], { cwd: '/' })).rejects.toThrow('key needs --device');
  });

  it('refuses a key it does not know before it connects', async () => {
    await expect(keyCommand.run(['Sideways', '--device', HOST], { cwd: '/' })).rejects.toThrow();
    expect(openRemote).not.toHaveBeenCalled();
  });
});
