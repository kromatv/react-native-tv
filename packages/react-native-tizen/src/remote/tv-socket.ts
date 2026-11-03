declare global {
  interface WebSocketInit {
    readonly tls?: { readonly rejectUnauthorized: boolean };
  }
}

/** What a remote session hears from its connection. */
export interface TvConnectionEvents {
  onMessage(text: string): void;
  onClosed(reason: string): void;
}

/** A WebSocket to a TV's remote API, reduced to what a session uses. */
export interface TvConnection {
  send(text: string): void;
  close(): void;
  bufferedAmount(): number;
}

/** Opens a {@link TvConnection}; tests pass their own. */
export type OpenTvConnection = (url: string, events: TvConnectionEvents) => TvConnection;

/**
 * Opens a WebSocket that accepts the TV's self-signed certificate, through the
 * `tls` option of Bun's WebSocket, which the CLI runs on.
 */
export const openTvConnection: OpenTvConnection = (url, events) => {
  const init: WebSocketInit = { tls: { rejectUnauthorized: false } };
  const socket = new WebSocket(url, init);
  socket.addEventListener('message', (event) => events.onMessage(String(event.data)));
  socket.addEventListener('close', (event) =>
    events.onClosed(event.reason === '' ? `closed with code ${event.code}` : event.reason),
  );
  socket.addEventListener('error', () => events.onClosed('the connection failed'));
  return {
    send: (text) => socket.send(text),
    close: () => socket.close(),
    bufferedAmount: () => socket.bufferedAmount,
  };
};
