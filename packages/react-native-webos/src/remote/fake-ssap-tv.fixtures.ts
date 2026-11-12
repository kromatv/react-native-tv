import { z } from 'zod';
import { type WebSocket, WebSocketServer } from 'ws';

export interface FakeSsapTvOptions {
  /** What happens to the connection request of a client the TV does not know. */
  readonly prompt?: 'accept' | 'decline' | 'ignore';
  /** Client keys the TV issued before. */
  readonly knownKeys?: readonly string[];
  /** `false` is a TV that accepts the socket and never answers. */
  readonly answers?: boolean;
  readonly releaseVersion?: string;
}

/** A local WebSocket server that answers as an LG TV's remote control service (webos-remote-control.md). */
export interface FakeSsapTv {
  readonly port: number;
  readonly uuid: string;
  /** The key the TV issues to a client it accepts. */
  readonly issuedKey: string;
  /** Every JSON message a client sent on the main socket. */
  readonly frames: Record<string, unknown>[];
  /** Every message a client sent on the pointer input socket, as written. */
  readonly pointerFrames: string[];
  close(): Promise<void>;
}

const UUID = '6f2c1c1e-8b0a-4c57-9d0e-2a7b3a4f5e61';
const ISSUED_KEY = '0f1e2d3c4b5a69788796a5b4c3d2e1f0';
const POINTER_PATH = '/resources/1a2b3c/netinput.pointer.sock';
const INPUT_PERMISSIONS = ['CONTROL_INPUT_TEXT', 'CONTROL_MOUSE_AND_KEYBOARD'];

const frameSchema = z.looseObject({
  id: z.union([z.string(), z.number()]).optional(),
  type: z.string(),
  uri: z.string().optional(),
  payload: z
    .looseObject({
      'client-key': z.string().optional(),
      manifest: z
        .looseObject({
          permissions: z.array(z.string()).optional(),
          signatures: z.unknown().optional(),
        })
        .optional(),
    })
    .optional(),
});
type Frame = z.infer<typeof frameSchema>;

interface Client {
  registered: boolean;
  canPress: boolean;
}

function register(tv: FakeSsapTv, options: FakeSsapTvOptions, client: Client, frame: Frame) {
  const { id, payload } = frame;
  const manifest = payload?.manifest;
  if (manifest?.signatures !== undefined) {
    return [
      {
        type: 'error',
        id,
        error: '403 Pairing rejected: blacklisted certificate detected',
        payload: {},
      },
    ];
  }
  const accepted = { type: 'registered', id, payload: { 'client-key': tv.issuedKey } };
  const known = [...(options.knownKeys ?? []), tv.issuedKey].includes(
    payload?.['client-key'] ?? '',
  );
  const prompt = options.prompt ?? 'accept';
  if (known || prompt === 'accept') {
    client.registered = true;
    client.canPress = INPUT_PERMISSIONS.every((each) => manifest?.permissions?.includes(each));
  }
  if (known) return [{ ...accepted, payload: { 'client-key': payload?.['client-key'] } }];
  const prompted = { type: 'response', id, payload: { pairingType: 'PROMPT', returnValue: true } };
  if (prompt === 'ignore') return [prompted];
  if (prompt === 'decline')
    return [prompted, { type: 'error', id, error: '403 User denied access', payload: {} }];
  return [prompted, accepted];
}

function request(tv: FakeSsapTv, client: Client, frame: Frame): object {
  const { id, uri } = frame;
  const error = (text: string) => ({ type: 'error', id, error: text, payload: {} });
  if (uri === 'ssap://system/getSystemInfo') {
    return {
      type: 'response',
      id,
      payload: {
        returnValue: true,
        features: { dvr: true },
        receiverType: 'DVB',
        modelName: 'OLED65C25LB',
        serialNumber: '000SERIAL000',
        programMode: true,
      },
    };
  }
  if (!client.registered) return error('401 insufficient permissions (not registered)');
  if (uri !== 'ssap://com.webos.service.networkinput/getPointerInputSocket')
    return error('404 no such service or method');
  if (!client.canPress) return error('401 insufficient permissions');
  const socketPath = `ws://127.0.0.1:${tv.port}${POINTER_PATH}`;
  return { type: 'response', id, payload: { socketPath, returnValue: true } };
}

function answer(tv: FakeSsapTv, options: FakeSsapTvOptions, client: Client, frame: Frame) {
  if (frame.type === 'hello') {
    return [
      {
        type: 'hello',
        payload: {
          protocolVersion: 1,
          deviceType: 'tv',
          deviceOS: 'webOS',
          deviceOSVersion: '4.1.0',
          deviceOSReleaseVersion: options.releaseVersion ?? '11.2.0',
          deviceUUID: tv.uuid,
          pairingTypes: ['PIN', 'PROMPT', 'COMBINED', 'LGSWITCH-PIN'],
        },
      },
    ];
  }
  if (frame.type === 'register') return register(tv, options, client, frame);
  return [request(tv, client, frame)];
}

/** Starts a fake TV remote control service on a free local port, without TLS. */
export async function fakeSsapTv(options: FakeSsapTvOptions = {}): Promise<FakeSsapTv> {
  const server = new WebSocketServer({ host: '127.0.0.1', port: 0 });
  await new Promise<void>((resolve) => server.once('listening', resolve));
  const address = server.address();
  if (address === null || typeof address === 'string') throw new Error('the fake TV got no port');
  const tv: FakeSsapTv = {
    port: address.port,
    uuid: UUID,
    issuedKey: ISSUED_KEY,
    frames: [],
    pointerFrames: [],
    close: () =>
      new Promise((resolve) => {
        for (const socket of server.clients) socket.terminate();
        server.close(() => resolve());
      }),
  };
  server.on('connection', (socket: WebSocket, incoming) => {
    if (incoming.headers.origin !== undefined) {
      socket.close(1008, 'invalid origin');
      return;
    }
    if (incoming.url === POINTER_PATH) {
      socket.on('message', (data) => tv.pointerFrames.push(String(data)));
      return;
    }
    const client: Client = { registered: false, canPress: false };
    socket.on('message', (data) => {
      const frame = frameSchema.parse(JSON.parse(String(data)));
      tv.frames.push(frame);
      if (options.answers === false) return;
      for (const reply of answer(tv, options, client, frame)) socket.send(JSON.stringify(reply));
    });
  });
  return tv;
}
