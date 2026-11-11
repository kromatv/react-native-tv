import { z } from 'zod';
import { openTvConnection, type OpenTvConnection, type TvConnection } from './tv-socket.ts';

/** The ports of a TV's remote control service: TLS, and the plain one older TVs still answer on. */
export interface SsapPorts {
  readonly secure: number;
  readonly plain: number;
}

export const SSAP_PORTS: SsapPorts = { secure: 3001, plain: 3000 };

const MAX_QUEUED_FRAMES = 16;

const frameSchema = z.object({
  type: z.string().max(64),
  id: z.union([z.string().max(128), z.number()]).optional(),
  error: z.string().max(1024).optional(),
  payload: z.unknown().optional(),
});
const payloadSchema = z.looseObject({
  returnValue: z.boolean().optional(),
  errorText: z.string().max(1024).optional(),
});

/** One message of the TV, by its `type`: `response`, `registered`, `hello` or `error`. */
export type SsapFrame = z.infer<typeof frameSchema>;

/** The TV's own refusal of a request, such as `401 insufficient permissions`. */
export class SsapError extends Error {
  constructor(text: string) {
    super(text);
    this.name = 'SsapError';
  }
}

/** The connection closed, or the TV did not answer in time. */
export class SsapSilentError extends Error {}

/** How {@link connectSsap} reaches a TV. */
export interface SsapConnectOptions {
  readonly timeoutMs: number;
  readonly ports?: SsapPorts;
  readonly open?: OpenTvConnection;
}

/** An open connection to a TV's remote control service. */
export interface SsapClient {
  /** The URL that answered: `wss` on 3001, else `ws` on 3000. */
  readonly url: string;
  send(frame: { readonly id: string; readonly type: string } & Record<string, unknown>): void;
  /** The next message carrying `id`; a `hello` answer, which has none, goes by `hello`. Throws the TV's `error`. */
  next(id: string, timeoutMs: number): Promise<SsapFrame>;
  /** Sends a request and returns the payload of its answer, throwing when the TV refuses it. */
  request(uri: string, payload?: object, timeoutMs?: number): Promise<unknown>;
  close(): void;
}

interface Waiter {
  resolve(frame: SsapFrame): void;
  reject(error: Error): void;
}

function parseFrame(text: string): SsapFrame | null {
  try {
    const parsed = frameSchema.safeParse(JSON.parse(text));
    return parsed.success ? parsed.data : null;
  } catch {
    return null;
  }
}

function client(
  url: string,
  connection: TvConnection,
  hooks: Hooks,
  defaultTimeoutMs: number,
): SsapClient {
  const queued = new Map<string, SsapFrame[]>();
  const waiting = new Map<string, Waiter>();
  let closed: string | null = null;
  let requests = 0;
  hooks.onMessage = (text) => {
    const frame = parseFrame(text);
    if (frame === null) return;
    const id = String(frame.id ?? frame.type);
    const waiter = waiting.get(id);
    if (waiter !== undefined) {
      waiting.delete(id);
      waiter.resolve(frame);
      return;
    }
    const frames = queued.get(id);
    if (frames !== undefined && frames.length < MAX_QUEUED_FRAMES) frames.push(frame);
  };
  hooks.onClosed = (reason) => {
    closed ??= reason;
    for (const waiter of waiting.values()) waiter.reject(new SsapSilentError(reason));
    waiting.clear();
  };
  const next = async (id: string, timeoutMs: number): Promise<SsapFrame> => {
    const frame =
      queued.get(id)?.shift() ??
      (await new Promise<SsapFrame>((resolve, reject) => {
        if (closed !== null) {
          reject(new SsapSilentError(closed));
          return;
        }
        const timer = setTimeout(() => {
          waiting.delete(id);
          reject(new SsapSilentError(`no answer within ${timeoutMs / 1000} s`));
        }, timeoutMs);
        waiting.set(id, {
          resolve: (answer) => {
            clearTimeout(timer);
            resolve(answer);
          },
          reject: (error) => {
            clearTimeout(timer);
            reject(error);
          },
        });
      }));
    if (frame.type === 'error') throw new SsapError(frame.error ?? 'an error');
    return frame;
  };
  const send: SsapClient['send'] = (frame) => {
    if (closed !== null) throw new SsapSilentError(closed);
    if (!queued.has(frame.id)) queued.set(frame.id, []);
    connection.send(JSON.stringify(frame));
  };
  return {
    url,
    send,
    next,
    async request(uri, payload = {}, timeoutMs = defaultTimeoutMs) {
      requests += 1;
      const id = `rn-webos-${requests}`;
      send({ id, type: 'request', uri, payload });
      const answer = payloadSchema.safeParse((await next(id, timeoutMs)).payload);
      if (answer.success && answer.data.returnValue === false)
        throw new SsapError(answer.data.errorText ?? `${uri} failed`);
      return answer.success ? answer.data : {};
    },
    close: () => connection.close(),
  };
}

interface Hooks {
  onMessage(text: string): void;
  onClosed(reason: string): void;
}

function connectTo(url: string, options: SsapConnectOptions): Promise<SsapClient> {
  const open = options.open ?? openTvConnection;
  return new Promise((resolve, reject) => {
    const hooks: Hooks = {
      onMessage: () => undefined,
      onClosed: (reason) => reject(new SsapSilentError(reason)),
    };
    const timer = setTimeout(() => {
      connection.close();
      reject(new SsapSilentError('the connection timed out'));
    }, options.timeoutMs);
    const connection = open(
      url,
      {
        onOpen: () => {
          clearTimeout(timer);
          resolve(client(url, connection, hooks, options.timeoutMs));
        },
        onMessage: (text) => hooks.onMessage(text),
        onClosed: (reason) => {
          clearTimeout(timer);
          hooks.onClosed(reason);
        },
      },
      options.timeoutMs,
    );
  });
}

/**
 * Connects to the remote control service of the TV at `host`: TLS on 3001
 * first, which 2022 and later TVs require, then the plain port of older ones.
 */
export async function connectSsap(host: string, options: SsapConnectOptions): Promise<SsapClient> {
  const ports = options.ports ?? SSAP_PORTS;
  try {
    return await connectTo(`wss://${host}:${ports.secure}`, options);
  } catch {
    return connectTo(`ws://${host}:${ports.plain}`, options);
  }
}
