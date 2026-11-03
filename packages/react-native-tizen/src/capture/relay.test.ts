import { createServer, type Server, type Socket } from 'node:net';
import { afterEach, describe, expect, it } from 'vitest';
import { FixableError } from '../errors.ts';
import { encodeFailure, encodeFrame } from './capture-protocol.ts';
import { captureThroughRelay } from './relay.ts';

const servers: Server[] = [];

function relay(answer: (socket: Socket) => void): Promise<number> {
  return new Promise((resolve, reject) => {
    const server = createServer((socket) => socket.once('data', () => answer(socket)));
    servers.push(server);
    server.listen(0, '127.0.0.1', () => {
      const address = server.address();
      if (address === null || typeof address === 'string') reject(new Error('no TCP port'));
      else resolve(address.port);
    });
  });
}

afterEach(async () => {
  await Promise.all(servers.splice(0).map((server) => new Promise((done) => server.close(done))));
});

describe('captureThroughRelay', () => {
  it('returns the frame the rn-tizen holding the port relays', async () => {
    const pixels = Buffer.alloc(8, 7);
    const port = await relay((socket) =>
      socket.end(encodeFrame(0, { width: 1, height: 2, stride: 4, pixels })),
    );

    await expect(captureThroughRelay(port, '192.168.1.50', 1000)).resolves.toEqual({
      width: 1,
      height: 2,
      stride: 4,
      pixels,
    });
  });

  it("passes on the app's failure with its hint", async () => {
    const port = await relay((socket) => socket.end(encodeFailure(0, 'no window', 'Start it')));

    await expect(captureThroughRelay(port, null, 1000)).rejects.toEqual(
      new FixableError('no window', 'Start it'),
    );
  });

  it('says the listener did not answer when it sends something else, or nothing', async () => {
    const chatty = await relay((socket) => socket.end('{"level":"info","message":"hi"}\n'));
    const silent = await relay((socket) => socket.end());

    await expect(captureThroughRelay(chatty, null, 1000)).rejects.toThrow(
      `The program listening on port ${chatty} did not answer the screenshot request`,
    );
    await expect(captureThroughRelay(silent, null, 1000)).rejects.toThrow('did not answer');
  });

  it('gives up on a listener that keeps the request waiting', async () => {
    const port = await relay(() => undefined);

    await expect(captureThroughRelay(port, null, 50)).rejects.toThrow('did not answer');
  });

  it('says nothing listens when it cannot connect', async () => {
    await expect(captureThroughRelay(0, null, 1000)).rejects.toThrow(
      'Cannot reach port 0 on this computer',
    );
  });
});
