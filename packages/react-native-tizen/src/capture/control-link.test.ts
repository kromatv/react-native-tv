import { createServer, type Server, Socket } from 'node:net';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { FixableError } from '../errors.ts';
import { ControlLink } from './control-link.ts';

let server: Server;
let socket: Socket;
let received: string;

function reply(value: unknown, payload: Buffer | null = null) {
  return { line: JSON.stringify(value), payload };
}

beforeEach(async () => {
  received = '';
  server = createServer((peer) => {
    peer.on('data', (chunk) => {
      received += String(chunk);
    });
  });
  await new Promise<void>((resolve) => server.listen(0, '127.0.0.1', resolve));
  const address = server.address();
  if (address === null || typeof address === 'string') throw new Error('no TCP port');
  socket = new Socket();
  await new Promise<void>((resolve) => socket.connect(address.port, '127.0.0.1', resolve));
});

afterEach(async () => {
  vi.useRealTimers();
  socket.destroy();
  await new Promise((resolve) => server.close(resolve));
});

describe('ControlLink', () => {
  it('resolves a capture with the frame the app sends for its id', async () => {
    const link = new ControlLink(socket, '192.168.1.50');
    const capturing = link.capture(1000);
    const pixels = Buffer.alloc(8);

    link.receive(
      reply({ type: 'screenshot', id: 1, width: 1, height: 2, stride: 4, bytes: 8 }, pixels),
    );

    await expect(capturing).resolves.toEqual({ width: 1, height: 2, stride: 4, pixels });
  });

  it('rejects a capture the app could not take, with its hint', async () => {
    const link = new ControlLink(socket, '192.168.1.50');
    const capturing = link.capture(1000);

    link.receive(
      reply({ type: 'screenshot-error', id: 1, message: 'no window', hint: 'Start it' }),
    );

    await expect(capturing).rejects.toEqual(
      new FixableError('The app could not capture its screen: no window', 'Start it'),
    );
  });

  it('ignores a reply to a capture nobody waits for', async () => {
    const link = new ControlLink(socket, '192.168.1.50');
    const capturing = link.capture(1000);

    link.receive(reply({ type: 'screenshot-error', id: 9, message: 'stale' }));
    link.closed();

    await expect(capturing).rejects.toThrow('disconnected before it answered');
  });

  it('drops the link and rejects every capture on a reply it cannot read', async () => {
    const link = new ControlLink(socket, '192.168.1.50');
    const first = link.capture(1000);
    const second = link.capture(1000);

    link.receive({ line: '{"type":"screenshot","id":"one"}', payload: null });

    await expect(first).rejects.toThrow('The app at 192.168.1.50 sent a bad reply');
    await expect(second).rejects.toThrow('sent a bad reply');
    expect(socket.destroyed).toBe(true);
  });

  it('gives up on a capture after its timeout', async () => {
    vi.useFakeTimers();
    const link = new ControlLink(socket, '192.168.1.50');

    const capturing = link.capture(2000);
    const failed = expect(capturing).rejects.toThrow('sent no screenshot within 2 s');
    await vi.advanceTimersByTimeAsync(2000);

    await failed;
  });

  it('writes the requests to the app', async () => {
    const link = new ControlLink(socket, '192.168.1.50');

    link.requestPerf(true);
    void link.capture(1000).catch(() => undefined);
    await vi.waitFor(() => expect(received.split('\n').filter(Boolean)).toHaveLength(2));
    link.closed();

    expect(received).toContain('screenshot');
  });
});
