import WebSocket from 'ws';

declare module 'ws' {
  interface ClientOptions {
    readonly tls?: { readonly rejectUnauthorized: boolean };
  }
}

const MAX_MESSAGE_BYTES = 64 * 1024;

function messageText(data: WebSocket.RawData): string {
  const chunks = Array.isArray(data) ? data : [data];
  return Buffer.concat(chunks.map((chunk) => new Uint8Array(chunk))).toString('utf8');
}

/** What a remote session hears from its connection. */
export interface TvConnectionEvents {
  onOpen?(): void;
  onMessage(text: string): void;
  onClosed(reason: string): void;
}

/** A WebSocket to a TV's remote control service, reduced to what a session uses. */
export interface TvConnection {
  send(text: string): void;
  close(): void;
  bufferedAmount(): number;
}

/** Opens a {@link TvConnection}; tests pass their own. */
export type OpenTvConnection = (
  url: string,
  events: TvConnectionEvents,
  timeoutMs?: number,
) => TvConnection;

/**
 * Opens a WebSocket that accepts the TV's own certificate, under Node and Bun
 * alike, and gives up when the TV does not finish the handshake in time.
 */
export const openTvConnection: OpenTvConnection = (url, events, timeoutMs) => {
  // No public CA signs the certificate an LG TV serves on 3001: every client turns the check off.
  // Node's ws reads rejectUnauthorized at the top level, Bun's built-in ws from tls.
  // ws sends no Origin header unless asked, and the TV closes a connection that has one.
  const socket = new WebSocket(url, {
    rejectUnauthorized: false,
    tls: { rejectUnauthorized: false },
    maxPayload: MAX_MESSAGE_BYTES,
    handshakeTimeout: timeoutMs,
  });
  socket.on('open', () => events.onOpen?.());
  socket.on('message', (data: WebSocket.RawData) => events.onMessage(messageText(data)));
  socket.on('close', (code: number, reason: Buffer) =>
    events.onClosed(reason.length === 0 ? `closed with code ${code}` : reason.toString()),
  );
  socket.on('error', (error: Error) => events.onClosed(error.message || 'the connection failed'));
  return {
    send: (text) => socket.send(text),
    close: () => socket.close(),
    bufferedAmount: () => socket.bufferedAmount,
  };
};
