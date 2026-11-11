import { Readable } from 'node:stream';
import { afterEach, describe, expect, it } from 'vitest';
import { fakeTv, type FakeTv, type FakeTvOptions } from './fake-tv.fixtures.ts';
import { openSshSession, SftpUnavailableError, SshConnectError } from './ssh-session.ts';

const tvs: FakeTv[] = [];

async function tv(options?: FakeTvOptions): Promise<FakeTv> {
  const started = await fakeTv(options);
  tvs.push(started);
  return started;
}

function target(started: FakeTv) {
  return { host: '127.0.0.1', port: started.port, privateKey: started.privateKey };
}

afterEach(async () => {
  await Promise.all(tvs.splice(0).map((started) => started.close()));
});

describe('openSshSession', () => {
  it('logs in as prisoner with the key and runs a command to its exit status', async () => {
    const started = await tv();

    const session = await openSshSession(target(started));
    const made = await session.exec('/usr/bin/test -d /media/developer/temp');
    const missing = await session.exec('/usr/bin/nothing');
    session.close();

    expect(made).toEqual({ code: 0, stdout: '', stderr: '' });
    expect(missing).toMatchObject({ code: 127, stderr: 'sh: /usr/bin/nothing: not found\n' });
  });

  it('talks to an sshd that only knows SHA-1 key exchange, ssh-rsa and aes128-ctr', async () => {
    const started = await tv({
      algorithms: {
        kex: ['diffie-hellman-group14-sha1'],
        serverHostKey: ['ssh-rsa'],
        cipher: ['aes128-ctr'],
        hmac: ['hmac-sha1'],
      },
    });

    const session = await openSshSession(target(started));
    const result = await session.exec('/usr/bin/test -d /tmp');
    session.close();

    expect(result.code).toBe(0);
  });

  it('says the key was refused, the port closed, or nothing there', async () => {
    const refusing = await tv({ acceptKey: false });
    const gone = await tv();
    await gone.close();

    const [denied, closed, garbled] = await Promise.allSettled([
      openSshSession(target(refusing)),
      openSshSession(target(gone)),
      openSshSession({ ...target(refusing), privateKey: 'not a key' }),
    ]);

    expect(denied).toMatchObject({ reason: { reason: 'denied' } });
    expect(closed).toMatchObject({ reason: { reason: 'closed' } });
    expect(garbled).toMatchObject({ reason: expect.any(SshConnectError) });
  });
});

describe('exec', () => {
  it('feeds a stream to the command and ends its input with it', async () => {
    const started = await tv();
    const session = await openSshSession(target(started));

    const result = await session.exec('/bin/cat > "/media/developer/temp/a.ipk"', {
      stdin: Readable.from([Buffer.from('pack'), Buffer.from('age')]),
    });
    session.close();

    expect(result.code).toBe(0);
    expect(started.files.get('/media/developer/temp/a.ipk')?.toString()).toBe('package');
  });

  it('ends a command that never ends by itself once a line settles it', async () => {
    const started = await tv();
    const session = await openSshSession(target(started));
    const states: string[] = [];

    const result = await session.exec(
      `/usr/bin/luna-send-pub -i luna://com.webos.appInstallService/dev/install '{"ipkUrl":"x"}'`,
      {
        onLine: (line) => {
          states.push(line);
          return line.includes('"installed"');
        },
      },
    );
    session.close();

    expect(states).toHaveLength(10);
    expect(result.stdout).toContain('"statusValue":30');
  });

  it('gives up on a command that outlasts its timeout', async () => {
    const started = await tv();
    const session = await openSshSession(target(started));

    const hanging = session.exec(
      `/usr/bin/luna-send-pub -i luna://com.webos.appInstallService/dev/install '{}'`,
      { timeoutMs: 50 },
    );

    await expect(hanging).rejects.toThrow('did not finish within 50 ms');
    session.close();
  });
});

describe('sftpPut', () => {
  it('writes a file through the SFTP subsystem and reports the bytes sent', async () => {
    const started = await tv();
    const session = await openSshSession(target(started));
    const sent: number[] = [];

    await session.sftpPut(Readable.from([Buffer.from('package')]), '/media/developer/temp/a.ipk', (bytes) =>
      sent.push(bytes),
    );
    session.close();

    expect(started.files.get('/media/developer/temp/a.ipk')?.toString()).toBe('package');
    expect(sent).toEqual([7]);
  });

  it('says so when the TV has no SFTP subsystem', async () => {
    const started = await tv({ sftp: false });
    const session = await openSshSession(target(started));

    const putting = session.sftpPut(Readable.from([Buffer.from('x')]), '/media/developer/temp/a.ipk');

    await expect(putting).rejects.toBeInstanceOf(SftpUnavailableError);
    session.close();
  });
});
