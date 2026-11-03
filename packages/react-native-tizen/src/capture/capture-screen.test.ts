import { connect, createServer, type Socket } from 'node:net';
import { afterEach, describe, expect, it } from 'vitest';
import type { LogEntry } from '../logs/log-line.ts';
import { listenForLogs, type LogServer } from '../logs/log-server.ts';
import { encodeFrame, type ScreenFrame } from './capture-protocol.ts';
import { captureScreen } from './capture-screen.ts';

const FRAME: ScreenFrame = { width: 2, height: 1, stride: 8, pixels: Buffer.from('BGRAbgra') };
const opened: { close(): unknown }[] = [];

function freePort(): Promise<number> {
  return new Promise((resolve) => {
    const probe = createServer().listen(0, () => {
      const address = probe.address();
      probe.close(() =>
        resolve(typeof address === 'object' && address !== null ? address.port : 0),
      );
    });
  });
}

function answerRequests(socket: Socket): void {
  let pending = '';
  socket.on('data', (chunk) => {
    const lines = (pending + chunk.toString()).split('\n');
    pending = lines.pop() ?? '';
    for (const line of lines) {
      const { id } = JSON.parse(line) as { id: number };
      socket.write(encodeFrame(id, FRAME));
    }
  });
}

function fakeApp(port: number): void {
  const socket = connect(port, '127.0.0.1');
  socket.on('connect', () => {
    socket.write('{"type":"control","version":1}\n');
    answerRequests(socket);
  });
  socket.on('error', () => setTimeout(() => fakeApp(port), 20));
  opened.push({ close: () => socket.destroy() });
}

async function listen(entries: LogEntry[] = [], connections: string[] = []): Promise<LogServer> {
  const server = await listenForLogs(0, {
    onEntry: (entry) => entries.push(entry),
    onConnect: (from) => connections.push(from),
  });
  opened.push(server);
  return server;
}

afterEach(async () => {
  for (const item of opened.splice(0).reverse()) await item.close();
});

describe('captureScreen', () => {
  it('listens on the port itself when nothing holds it, until the app connects', async () => {
    const port = await freePort();

    const capturing = captureScreen(port, null);
    fakeApp(port);

    expect(await capturing).toEqual(FRAME);
  });

  it('asks the rn-tizen that holds the port, which asks the app', async () => {
    const server = await listen();
    fakeApp(server.port);

    expect(await captureScreen(server.port, '127.0.0.1')).toEqual(FRAME);
  });

  it('says no development build is connected when none comes', async () => {
    const server = await listen();

    await expect(server.capture('10.0.0.9', 50)).rejects.toThrow(
      `No development build on 10.0.0.9 is connected to port ${server.port}`,
    );
  });
});

describe('listenForLogs with a control link', () => {
  it('prints only log lines, and announces only log connections', async () => {
    const entries: LogEntry[] = [];
    const connections: string[] = [];
    const server = await listen(entries, connections);
    fakeApp(server.port);
    const logs = connect(server.port, '127.0.0.1', () =>
      logs.write('{"level":"warn","message":"hi"}\n'),
    );
    opened.push({ close: () => logs.destroy() });

    await server.capture(null, 1000);
    await expect.poll(() => entries).toEqual([{ level: 'warn', message: 'hi' }]);

    expect(connections).toEqual(['127.0.0.1']);
  });
});
