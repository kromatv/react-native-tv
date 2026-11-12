import { afterEach, describe, expect, it } from 'vitest';
import { type WebSocket, WebSocketServer } from 'ws';
import { openTvConnection } from './tv-socket.ts';

const servers: WebSocketServer[] = [];

function listen(onConnection: (socket: WebSocket) => void): Promise<string> {
  return new Promise((resolve, reject) => {
    const started = new WebSocketServer({ host: '127.0.0.1', port: 0 }, () => {
      const address = started.address();
      if (address === null || typeof address === 'string') reject(new Error('no TCP port'));
      else resolve(`ws://127.0.0.1:${address.port}/api/v2/channels/samsung.remote.control`);
    });
    started.on('connection', onConnection);
    servers.push(started);
  });
}

function closeReason(url: string): Promise<string> {
  return new Promise((resolve) => {
    openTvConnection(url, { onMessage: ignore, onClosed: resolve });
  });
}

function ignore(): void {
  return undefined;
}

afterEach(async () => {
  await Promise.all(
    servers.splice(0).map((server) => {
      for (const client of server.clients) client.terminate();
      return new Promise((done) => server.close(done));
    }),
  );
});

describe('openTvConnection', () => {
  it('hands each message to onMessage and sends what the session sends', async () => {
    const received: string[] = [];
    const url = await listen((socket) => {
      socket.on('message', (data) => received.push(String(data)));
      socket.send('{"event":"ms.channel.connect"}');
    });

    const message = await new Promise<string>((resolve) => {
      const connection = openTvConnection(url, {
        onMessage: (text) => {
          connection.send('{"method":"ms.remote.control"}');
          resolve(text);
        },
        onClosed: ignore,
      });
    });
    await new Promise((resolve) => setTimeout(resolve, 50));

    expect(message).toBe('{"event":"ms.channel.connect"}');
    expect(received).toEqual(['{"method":"ms.remote.control"}']);
  });

  it('reads a message the TV sends as a binary frame as text', async () => {
    const url = await listen((socket) => socket.send(Buffer.from('{"event":"ms.error"}')));

    const message = await new Promise<string>((resolve) => {
      openTvConnection(url, { onMessage: resolve, onClosed: ignore });
    });

    expect(message).toBe('{"event":"ms.error"}');
  });

  it('reports the reason the TV closed with, else its code', async () => {
    const url = await listen((socket) => socket.close(4000, 'refused'));
    const silent = await listen((socket) => socket.close(4001));

    const reasons = await Promise.all([url, silent].map(closeReason));

    expect(reasons).toEqual(['refused', 'closed with code 4001']);
  });

  it('reports a connection that fails', async () => {
    const reason = await closeReason('ws://127.0.0.1:1/');

    expect(reason).toMatch(/ECONNREFUSED|closed with code/);
  });

  it('closes on request, with nothing left to send', async () => {
    const url = await listen(() => undefined);

    const connection = openTvConnection(url, { onMessage: ignore, onClosed: ignore });
    connection.close();

    expect(connection.bufferedAmount()).toBe(0);
  });
});
