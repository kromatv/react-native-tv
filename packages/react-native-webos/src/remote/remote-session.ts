import { setTimeout as sleep } from 'node:timers/promises';
import { z } from 'zod';
import { FixableError } from '../errors.ts';
import { isClientKey, readClientKey, saveClientKey } from './client-key.ts';
import {
  connectSsap,
  SsapError,
  type SsapClient,
  type SsapConnectOptions,
  type SsapPorts,
} from './ssap-client.ts';
import { sayHello } from './ssap-hello.ts';
import { REGISTER_MANIFEST } from './ssap-manifest.ts';
import { openTvConnection, type OpenTvConnection, type TvConnection } from './tv-socket.ts';

const POINTER_SOCKET = 'ssap://com.webos.service.networkinput/getPointerInputSocket';
const REGISTER_ID = 'register_0';
const DRAIN_POLL_MS = 20;
const DRAIN_LIMIT_MS = 2_000;
const SETTLE_MS = 200;

const ACCEPT_HINT =
  'Accept the connection request the TV shows with its remote, then run the command again.';
const REACH_HINT =
  'Is the TV on, and on this network? Its Network settings must allow LG Connect Apps.';

const registeredSchema = z.object({ 'client-key': z.string().max(256).optional() });
const pointerSchema = z.object({ socketPath: z.url({ protocol: /^wss?$/ }).max(1024) });

/** Where the client keys TVs issue are kept; tests pass their own. */
export interface ClientKeyStore {
  read(tvId: string): string | null;
  save(tvId: string, key: string): void;
}

/** A TV that accepted `rn-webos` as a remote. */
export interface RemoteSession {
  /** Presses each button in order, `keyIntervalMs` apart, and waits until they are sent. */
  press(buttons: readonly string[]): Promise<void>;
  close(): void;
}

export interface OpenRemoteOptions {
  readonly host: string;
  readonly timeoutMs: number;
  /** How long somebody has to accept `rn-webos` on the TV. */
  readonly pairingTimeoutMs: number;
  readonly keyIntervalMs: number;
  /** Called when the TV shows its connection request and waits for an answer. */
  readonly onPrompt?: () => void;
  readonly keys?: ClientKeyStore;
  readonly ports?: SsapPorts;
  readonly open?: OpenTvConnection;
}

const STATE_KEYS: ClientKeyStore = { read: readClientKey, save: saveClientKey };

function reason(error: unknown): string {
  return error instanceof Error ? error.message : String(error);
}

function unreachable(host: string, error: unknown): FixableError {
  return new FixableError(
    `Cannot reach the remote control service of the TV at ${host} (${reason(error)})`,
    REACH_HINT,
  );
}

/** Registers with the stored client key, or waits for somebody to accept the TV's prompt. */
async function register(client: SsapClient, tvId: string, options: OpenRemoteOptions) {
  const { host } = options;
  const keys = options.keys ?? STATE_KEYS;
  const stored = keys.read(tvId);
  client.send({
    id: REGISTER_ID,
    type: 'register',
    payload: {
      forcePairing: false,
      pairingType: 'PROMPT',
      ...(stored === null ? {} : { 'client-key': stored }),
      manifest: REGISTER_MANIFEST,
    },
  });
  try {
    let answer = await client.next(REGISTER_ID, options.timeoutMs);
    if (answer.type !== 'registered') {
      options.onPrompt?.();
      answer = await client.next(REGISTER_ID, options.pairingTimeoutMs).catch((error: unknown) => {
        if (error instanceof SsapError) throw error;
        throw new FixableError(
          `Nobody accepted rn-webos on the TV at ${host} within ${options.pairingTimeoutMs / 1000} s`,
          ACCEPT_HINT,
        );
      });
    }
    const issued = registeredSchema.safeParse(answer.payload);
    const key = issued.success ? issued.data['client-key'] : undefined;
    if (key !== undefined && key !== stored && isClientKey(key)) keys.save(tvId, key);
  } catch (error) {
    if (error instanceof SsapError)
      throw new FixableError(`The TV at ${host} refused rn-webos: ${error.message}`, ACCEPT_HINT);
    throw error instanceof FixableError ? error : unreachable(host, error);
  }
}

function openPointer(url: string, options: OpenRemoteOptions): Promise<TvConnection> {
  const open = options.open ?? openTvConnection;
  return new Promise((resolve, reject) => {
    const connection = open(
      url,
      {
        onOpen: () => resolve(connection),
        onMessage: () => undefined,
        onClosed: (why) => reject(new Error(why)),
      },
      options.timeoutMs,
    );
  });
}

async function pointerSocket(client: SsapClient, options: OpenRemoteOptions) {
  const { host } = options;
  let answer: unknown;
  try {
    answer = await client.request(POINTER_SOCKET);
  } catch (error) {
    if (!(error instanceof SsapError)) throw unreachable(host, error);
    throw new FixableError(
      `The TV at ${host} did not open its remote input: ${error.message}`,
      'This TV may not let a network remote press keys.',
    );
  }
  const pointer = pointerSchema.safeParse(answer);
  if (!pointer.success) throw unreachable(host, 'it named no input socket');
  return openPointer(pointer.data.socketPath, options).catch((error: unknown) => {
    throw unreachable(host, error);
  });
}

function session(client: SsapClient, pointer: TvConnection, intervalMs: number): RemoteSession {
  return {
    async press(buttons) {
      for (const [index, button] of buttons.entries()) {
        if (index > 0) await sleep(intervalMs);
        pointer.send(`type:button\nname:${button}\n\n`);
      }
      for (
        let waited = 0;
        pointer.bufferedAmount() > 0 && waited < DRAIN_LIMIT_MS;
        waited += DRAIN_POLL_MS
      ) {
        await sleep(DRAIN_POLL_MS);
      }
      await sleep(SETTLE_MS);
    },
    close() {
      pointer.close();
      client.close();
    },
  };
}

/**
 * Connects to the TV as a remote. A TV that does not know `rn-webos` yet asks
 * its viewer to accept it, and the client key it then issues is kept for the
 * next time, under the TV's own id. Rejects when the TV refuses or stays silent.
 */
export async function openRemote(options: OpenRemoteOptions): Promise<RemoteSession> {
  const { host, timeoutMs } = options;
  const connect: SsapConnectOptions = { timeoutMs, ports: options.ports, open: options.open };
  const client = await connectSsap(host, connect).catch((error: unknown) => {
    throw unreachable(host, error);
  });
  try {
    const hello = await sayHello(client, timeoutMs).catch((error: unknown) => {
      throw unreachable(host, error);
    });
    await register(client, hello.uuid ?? host, options);
    const pointer = await pointerSocket(client, options);
    return session(client, pointer, options.keyIntervalMs);
  } catch (error) {
    client.close();
    throw error;
  }
}
