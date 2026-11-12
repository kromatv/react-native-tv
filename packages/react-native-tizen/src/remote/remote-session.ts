import { setTimeout as sleep } from 'node:timers/promises';
import { z } from 'zod';
import { FixableError } from '../errors.ts';
import { isRemoteToken } from './remote-token.ts';
import { openTvConnection, type OpenTvConnection, type TvConnection } from './tv-socket.ts';

const REMOTE_PORT = 8002;
const CLIENT_NAME = 'rn-tizen';
const DRAIN_POLL_MS = 20;
const DRAIN_LIMIT_MS = 2_000;
const SETTLE_MS = 200;

/** The menu where a TV lists the clients allowed to use its remote API. */
export const DEVICE_LIST_PATH =
  'Settings > General & Privacy > External Device Manager > Device Connection Manager > Device List';

const eventSchema = z.object({ event: z.string(), data: z.unknown().optional() });
const connectDataSchema = z.object({ token: z.union([z.string(), z.number()]).optional() });
const errorDataSchema = z.object({ message: z.string().max(4096) });

/** A connection to a TV's remote API that the TV accepted. */
export interface RemoteSession {
  /** The token the TV issued on this connection, null when it sent none. */
  readonly token: string | null;
  /** Clicks each `KEY_*` code in order, `keyIntervalMs` apart, and waits until they are sent. */
  press(codes: readonly string[]): Promise<void>;
  close(): void;
}

interface OpenRemoteOptions {
  readonly host: string;
  readonly token: string | null;
  readonly timeoutMs: number;
  readonly keyIntervalMs: number;
  readonly open?: OpenTvConnection;
}

/** The address of the TV's remote API, with the name the TV shows and the token it issued. */
export function remoteUrl(host: string, token: string | null): string {
  const name = Buffer.from(CLIENT_NAME).toString('base64');
  const auth = token === null ? '' : `&token=${token}`;
  return `wss://${host}:${REMOTE_PORT}/api/v2/channels/samsung.remote.control?name=${name}${auth}`;
}

function parseJson(text: string): unknown {
  try {
    return JSON.parse(text);
  } catch {
    return null;
  }
}

function clickMessage(code: string): string {
  return JSON.stringify({
    method: 'ms.remote.control',
    params: { Cmd: 'Click', DataOfCmd: code, Option: 'false', TypeOfRemote: 'SendRemoteKey' },
  });
}

function issuedToken(data: unknown): string | null {
  const parsed = connectDataSchema.safeParse(data);
  const token = parsed.success ? parsed.data.token : undefined;
  if (token === undefined) return null;
  const text = String(token);
  return isRemoteToken(text) ? text : null;
}

function refused(host: string): FixableError {
  return new FixableError(
    `The TV at ${host} refused rn-tizen`,
    `On the TV, open ${DEVICE_LIST_PATH}: allow rn-tizen, or delete it to be asked again.`,
  );
}

function rejection(host: string, event: string, data: unknown): FixableError | null {
  if (event === 'ms.channel.unauthorized') return refused(host);
  if (event === 'ms.channel.timeOut') {
    return new FixableError(
      `The TV at ${host} did not accept the connection`,
      'Is it on, with this computer on its network, not a guest network or another VLAN?',
    );
  }
  if (event === 'ms.error') {
    const parsed = errorDataSchema.safeParse(data);
    const detail = parsed.success ? parsed.data.message : 'an error';
    return new FixableError(
      `The TV at ${host} answered ${detail}`,
      'This TV may not support the network remote.',
    );
  }
  return null;
}

function unreachable(host: string, reason: string): FixableError {
  return new FixableError(
    `Cannot reach the remote API of the TV at ${host}:${REMOTE_PORT} (${reason})`,
    'Is the TV on, and on this network? Its remote API answers once it has started.',
  );
}

function noAnswer(host: string, pairing: boolean, timeoutMs: number): FixableError {
  const seconds = timeoutMs / 1000;
  if (!pairing) {
    return new FixableError(`The TV at ${host} did not answer within ${seconds} s`, 'Is it on?');
  }
  return new FixableError(
    `Nobody allowed rn-tizen on the TV within ${seconds} s`,
    'Choose Allow when the TV asks about rn-tizen, then run the command again.',
  );
}

function session(
  connection: TvConnection,
  token: string | null,
  options: OpenRemoteOptions,
  closed: () => string | null,
): RemoteSession {
  const ensureOpen = () => {
    const reason = closed();
    if (reason !== null) throw unreachable(options.host, reason);
  };
  return {
    token,
    async press(codes) {
      for (const [index, code] of codes.entries()) {
        if (index > 0) await sleep(options.keyIntervalMs);
        ensureOpen();
        connection.send(clickMessage(code));
      }
      for (
        let waited = 0;
        connection.bufferedAmount() > 0 && waited < DRAIN_LIMIT_MS;
        waited += DRAIN_POLL_MS
      ) {
        await sleep(DRAIN_POLL_MS);
      }
      await sleep(SETTLE_MS);
      ensureOpen();
    },
    close: () => connection.close(),
  };
}

/**
 * Connects to the TV's remote API. Without a token the TV asks its viewer to
 * allow rn-tizen, and the session carries the token it then issues. Rejects
 * when the TV refuses, or answers nothing within `timeoutMs`.
 */
export function openRemote(options: OpenRemoteOptions): Promise<RemoteSession> {
  const { host } = options;
  const open = options.open ?? openTvConnection;
  return new Promise((resolve, reject) => {
    let closedReason: string | null = null;
    let connection: TvConnection | null = null;
    let settled = false;
    const timer = setTimeout(
      () => fail(noAnswer(host, options.token === null, options.timeoutMs)),
      options.timeoutMs,
    );
    const settle = (outcome: () => void) => {
      if (settled) return;
      settled = true;
      clearTimeout(timer);
      outcome();
    };
    const fail = (error: FixableError) =>
      settle(() => {
        connection?.close();
        reject(error);
      });
    const onMessage = (text: string) => {
      const parsed = eventSchema.safeParse(parseJson(text));
      if (!parsed.success || connection === null) return;
      const { event, data } = parsed.data;
      if (event === 'ms.channel.connect') {
        const accepted = session(connection, issuedToken(data), options, () => closedReason);
        settle(() => resolve(accepted));
        return;
      }
      const error = rejection(host, event, data);
      if (error !== null) fail(error);
    };
    const onClosed = (reason: string) => {
      closedReason ??= reason;
      fail(unreachable(host, reason));
    };
    connection = open(remoteUrl(host, options.token), { onMessage, onClosed });
  });
}
