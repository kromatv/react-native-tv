import { connect } from 'node:net';
import { networkInterfaces } from 'node:os';
import { afterEach, describe, expect, it, vi } from 'vitest';
import { relayRequest } from '../capture/capture-protocol.ts';
import type { LogEntry } from './log-line.ts';
import { acceptsPeer, LINK_WAIT_MS, listenForLogs } from './log-server.ts';

const LAN_ADDRESS = Object.values(networkInterfaces())
  .flat()
  .find((entry) => entry?.family === 'IPv4' && !entry.internal)?.address;

function exchange(port: number, host: string, request: string): Promise<string> {
  return new Promise((resolve, reject) => {
    let answer = '';
    const socket = connect(port, host, () => socket.write(request));
    socket.on('data', (chunk) => {
      answer += String(chunk);
    });
    socket.on('close', () => resolve(answer));
    socket.on('error', reject);
  });
}

function send(port: number, payload: string): Promise<void> {
  return new Promise((resolve, reject) => {
    const socket = connect(port, '127.0.0.1', () => socket.end(payload));
    socket.on('close', () => resolve());
    socket.on('error', reject);
  });
}

describe('listenForLogs', () => {
  it('reads every line a TV streams, the unterminated last one included', async () => {
    const entries: LogEntry[] = [];
    let markDisconnected: (() => void) | undefined;
    const disconnected = new Promise<void>((resolve) => {
      markDisconnected = resolve;
    });
    const server = await listenForLogs(0, {
      onEntry: (entry) => entries.push(entry),
      onDisconnect: () => markDisconnected?.(),
    });

    await send(server.port, '{"level":"info","message":"one"}\n{"level":"error","message":"two"}');
    await disconnected;
    await server.close();

    expect(entries).toEqual([
      { level: 'info', message: 'one' },
      { level: 'error', message: 'two' },
    ]);
  });

  it('refuses a port another listener holds', async () => {
    const first = await listenForLogs(0, { onEntry: () => undefined });

    await expect(listenForLogs(first.port, { onEntry: () => undefined })).rejects.toThrow(
      `Port ${first.port} is already in use`,
    );

    await first.close();
  });
});

describe('listenForLogs relaying a screenshot', () => {
  afterEach(() => {
    vi.useRealTimers();
  });

  it('answers a relay request with why it found no app to capture', async () => {
    vi.useFakeTimers({ toFake: ['setTimeout', 'clearTimeout'] });
    const server = await listenForLogs(0, { onEntry: () => undefined });

    const answering = exchange(server.port, '127.0.0.1', relayRequest('192.168.1.50', null));
    await vi.waitFor(() => expect(vi.getTimerCount()).toBe(1), { interval: 5 });
    await vi.advanceTimersByTimeAsync(LINK_WAIT_MS);
    const answer = await answering;
    await server.close();

    expect(answer).toContain('No development build on 192.168.1.50 is connected to port');
    expect(answer).toContain('screenshot-error');
  });
});

describe('listenForLogs once the TV is known', () => {
  it.runIf(LAN_ADDRESS !== undefined)('drops a connection from another host', async () => {
    const entries: LogEntry[] = [];
    const server = await listenForLogs(
      0,
      { onEntry: (entry) => entries.push(entry) },
      { peer: '203.0.113.7' },
    );

    const outcome = await exchange(
      server.port,
      LAN_ADDRESS ?? '',
      '{"level":"info","message":"x"}\n',
    ).catch((error: NodeJS.ErrnoException) => error.code);
    await server.close();

    expect(['', 'ECONNRESET']).toContain(outcome);
    expect(entries).toEqual([]);
  });
});

describe('listenForLogs with a perf listener', () => {
  it('asks each app that opens a control link for perf samples', async () => {
    const server = await listenForLogs(0, { wantsPerf: true, onEntry: () => undefined });
    const socket = connect(server.port, '127.0.0.1');

    const request = await new Promise<string>((resolve) => {
      socket.on('connect', () => socket.write('{"type":"control","version":1}\n'));
      socket.on('data', (chunk) => resolve(chunk.toString()));
    });
    socket.destroy();
    await server.close();

    expect(request).toBe('{"type":"perf","on":true}\n');
  });

  it('tells the listener which address each entry came from', async () => {
    const sources: string[] = [];
    let markDisconnected: (() => void) | undefined;
    const disconnected = new Promise<void>((resolve) => {
      markDisconnected = resolve;
    });
    const server = await listenForLogs(0, {
      onEntry: (_entry, from) => sources.push(from),
      onDisconnect: () => markDisconnected?.(),
    });

    await send(server.port, '{"level":"info","message":"one"}\n');
    await disconnected;
    await server.close();

    expect(sources).toEqual(['127.0.0.1']);
  });
});

describe('listenForLogs with two apps on one port', () => {
  const hello = (app: string) => `${JSON.stringify({ type: 'log', version: 1, app })}\n`;

  it('names the app of each line, and reads a build that names none', async () => {
    const heard: string[] = [];
    const connected: (string | null)[] = [];
    const server = await listenForLogs(0, {
      onEntry: (entry, _from, app) => heard.push(`${app}: ${entry.message}`),
      onConnect: (_from, app) => connected.push(app),
    });

    await send(server.port, `${hello('org.example.a')}{"level":"info","message":"from a"}\n`);
    await send(server.port, '{"level":"info","message":"from an old build"}\n');
    await vi.waitFor(() => expect(heard).toHaveLength(2));
    await server.close();

    expect([...heard].sort()).toEqual(['null: from an old build', 'org.example.a: from a']);
    expect([...connected].sort()).toEqual(['org.example.a', null].sort());
  });

  it('ignores the lines and the control link of another app than the one asked for', async () => {
    const heard: string[] = [];
    const server = await listenForLogs(
      0,
      { onEntry: (entry, _from, app) => heard.push(`${app}: ${entry.message}`) },
      { app: 'org.example.a' },
    );

    await send(server.port, `${hello('org.example.b')}{"level":"info","message":"from b"}\n`);
    await send(server.port, `${hello('org.example.a')}{"level":"info","message":"from a"}\n`);
    await send(server.port, '{"type":"control","version":1,"app":"org.example.b"}\n');
    await vi.waitFor(() => expect(heard).toHaveLength(1));

    await expect(server.capture(null, 'org.example.b', 50)).rejects.toThrow(
      'No development build of org.example.b is connected',
    );
    await server.close();
    expect(heard).toEqual(['org.example.a: from a']);
  });
});

describe('acceptsPeer', () => {
  it('accepts anyone when the TV is not known', () => {
    expect(acceptsPeer('192.168.1.60', null)).toBe(true);
  });

  it('accepts the TV', () => {
    expect(acceptsPeer('192.168.1.50', '192.168.1.50')).toBe(true);
  });

  it('accepts this computer, which relays screenshots', () => {
    expect(acceptsPeer('127.0.0.1', '192.168.1.50')).toBe(true);
    expect(acceptsPeer('::1', '192.168.1.50')).toBe(true);
  });

  it('drops another host once the TV is known', () => {
    expect(acceptsPeer('192.168.1.60', '192.168.1.50')).toBe(false);
  });
});
