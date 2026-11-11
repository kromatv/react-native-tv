import { createPrivateKey } from 'node:crypto';
import { afterEach, describe, expect, it } from 'vitest';
import {
  fakeKeyServer,
  protectKey,
  rsaKeyPem,
  type FakeKeyServer,
} from './developer-key.fixtures.ts';
import { decryptDeveloperKey, fetchDeveloperKey } from './key-server.ts';

const servers: FakeKeyServer[] = [];

async function keyServer(body: string, status?: number): Promise<FakeKeyServer> {
  const server = await fakeKeyServer(body, status);
  servers.push(server);
  return server;
}

afterEach(async () => {
  await Promise.all(servers.splice(0).map((server) => server.close()));
});

describe('decryptDeveloperKey', () => {
  const key = rsaKeyPem();
  const served = protectKey(key, 'A1b2C3');

  it('opens the key as the TV protects it, with the passphrase on screen', () => {
    expect(served).toContain('Proc-Type: 4,ENCRYPTED\nDEK-Info: AES-128-CBC,');

    const opened = decryptDeveloperKey(served, 'A1b2C3');

    expect(opened).toMatch(/^-----BEGIN RSA PRIVATE KEY-----\n/);
    expect(opened).not.toContain('ENCRYPTED');
    expect(createPrivateKey(opened).equals(createPrivateKey(key))).toBe(true);
  });

  it('says the passphrase is wrong, not that the key is broken', () => {
    expect(() => decryptDeveloperKey(served, 'a1b2c3')).toThrow(
      'The passphrase does not open the key the TV served',
    );
  });
});

describe('fetchDeveloperKey', () => {
  it('reads the key at /webos_rsa', async () => {
    const pem = protectKey(rsaKeyPem(), 'A1b2C3');
    const server = await keyServer(pem);

    expect(await fetchDeveloperKey('127.0.0.1', server.port)).toBe(pem);
    expect(server.requests).toEqual(['GET /webos_rsa']);
  });

  it('asks for Key Server when the port is closed or answers another status', async () => {
    const closed = await keyServer('');
    await closed.close();
    const missing = await keyServer('', 404);

    await expect(fetchDeveloperKey('127.0.0.1', closed.port)).rejects.toMatchObject({
      message: `The key server of the TV at 127.0.0.1:${closed.port} does not answer`,
      hint: expect.stringContaining('turn Key Server on'),
    });
    await expect(fetchDeveloperKey('127.0.0.1', missing.port)).rejects.toThrow('answered HTTP 404');
  });

  it('refuses what is not a key, and more than a key can be', async () => {
    const page = await keyServer('<html>Hello world</html>');
    const huge = await keyServer(`-----BEGIN RSA PRIVATE KEY-----\n${'A'.repeat(70_000)}`);

    await expect(fetchDeveloperKey('127.0.0.1', page.port)).rejects.toThrow(
      'answered something that is not a key',
    );
    await expect(fetchDeveloperKey('127.0.0.1', huge.port)).rejects.toThrow(
      'answered something that is not a key',
    );
  });
});
