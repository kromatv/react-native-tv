const TIMEOUT_MS = 8000;

interface ReactNativeWebSocketConstructor {
  new (
    url: string,
    protocols: string[] | undefined,
    options: { readonly headers: Readonly<Record<string, string>> },
  ): WebSocket;
}

const ReactNativeWebSocket: ReactNativeWebSocketConstructor = WebSocket;

export interface Closed {
  readonly code: number;
  readonly reason: string;
}

/** A WebSocket whose messages and close can be awaited one at a time. */
export interface SocketSession {
  readonly socket: WebSocket;
  next(): Promise<unknown>;
  closed(): Promise<Closed>;
}

export function within<T>(label: string, pending: Promise<T>, timeoutMs = TIMEOUT_MS): Promise<T> {
  return new Promise<T>((resolve, reject) => {
    const timer = setTimeout(
      () => reject(new Error(`${label}: no answer in ${timeoutMs} ms`)),
      timeoutMs,
    );
    pending.then(
      (value) => {
        clearTimeout(timer);
        resolve(value);
      },
      (error: unknown) => {
        clearTimeout(timer);
        reject(error instanceof Error ? error : new Error(String(error)));
      },
    );
  });
}

/** Opens `url` and resolves once it is open; rejects with the close reason when it fails. */
export function openSession(
  url: string,
  protocols?: readonly string[],
  headers: Readonly<Record<string, string>> = {},
  binaryType: BinaryType = 'arraybuffer',
): Promise<SocketSession> {
  const socket = new ReactNativeWebSocket(url, protocols ? [...protocols] : undefined, { headers });
  socket.binaryType = binaryType;
  const messages: unknown[] = [];
  const waiting: ((data: unknown) => void)[] = [];
  let closeEvent: Closed | null = null;
  const closeWaiters: ((closed: Closed) => void)[] = [];
  socket.addEventListener('message', (event) => {
    const take = waiting.shift();
    if (take) take(event.data);
    else messages.push(event.data);
  });
  const opened = new Promise<SocketSession>((resolve, reject) => {
    socket.addEventListener('open', () =>
      resolve({
        socket,
        next: () =>
          within(
            'message',
            messages.length > 0
              ? Promise.resolve(messages.shift())
              : new Promise((take) => waiting.push(take)),
          ),
        closed: () =>
          within(
            'close',
            closeEvent
              ? Promise.resolve(closeEvent)
              : new Promise((take) => closeWaiters.push(take)),
          ),
      }),
    );
    socket.addEventListener('close', (event) => {
      closeEvent = { code: event.code, reason: event.reason };
      for (const take of closeWaiters.splice(0)) take(closeEvent);
      reject(new Error(`closed before opening: ${event.code} ${event.reason}`));
    });
  });
  return within('open', opened);
}

/** React Native's `WebSocket.ping()`, which the DOM type does not have. */
export function ping(socket: WebSocket): void {
  const method: unknown = Reflect.get(socket, 'ping');
  if (typeof method !== 'function') throw new Error('WebSocket.ping is missing');
  Reflect.apply(method, socket, []);
}
