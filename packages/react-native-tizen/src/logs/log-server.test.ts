import { connect } from 'node:net';
import { describe, expect, it } from 'vitest';
import type { LogEntry } from './log-line.ts';
import { acceptsPeer, listenForLogs } from './log-server.ts';

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
