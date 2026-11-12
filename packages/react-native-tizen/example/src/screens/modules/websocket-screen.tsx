import { attempt, check, CheckList, useChecks, type Check } from '../expo/check-list.tsx';
import { ECHO_HTTP, ECHO_SOCKET } from './echo-endpoint.ts';
import { readBlob } from './read-blob.ts';
import { openSession, ping, within } from './socket-session.ts';

const PUBLIC_WSS_ECHO = 'wss://ws.postman-echo.com/raw';
const LARGE_BYTES = 70_000;

function headersOf(greeting: unknown): Record<string, unknown> {
  if (typeof greeting !== 'string') throw new Error('The greeting is not text');
  const parsed: unknown = JSON.parse(greeting);
  const headers: unknown =
    typeof parsed === 'object' && parsed !== null ? Reflect.get(parsed, 'headers') : null;
  if (typeof headers !== 'object' || headers === null)
    throw new Error('The greeting has no headers');
  return Object.fromEntries(Object.entries(headers));
}

function sameBytes(left: Uint8Array, right: Uint8Array): boolean {
  return left.length === right.length && left.every((byte, index) => byte === right[index]);
}

async function webSocketChecks(): Promise<readonly Check[]> {
  return [
    await attempt('ws open, protocol, headers, Origin', async () => {
      const session = await openSession(ECHO_SOCKET, ['echo.v1', 'other'], { 'X-Probe': 'tizen' });
      const headers = headersOf(await session.next());
      session.socket.close();
      const expectedOrigin = ECHO_HTTP.replace(/\/$/, '');
      return check(
        'ws open, protocol, headers, Origin',
        session.socket.protocol === 'echo.v1' &&
          headers['x-probe'] === 'tizen' &&
          headers['origin'] === expectedOrigin,
        { protocol: session.socket.protocol, probe: headers['x-probe'], origin: headers['origin'] },
      );
    }),
    await attempt('ws text echo (UTF-8)', async () => {
      const session = await openSession(ECHO_SOCKET);
      await session.next();
      const text = 'héllo ✓ 🎉';
      session.socket.send(text);
      const echoed = await session.next();
      session.socket.close();
      return check('ws text echo (UTF-8)', echoed === text, echoed);
    }),
    await attempt(`ws binary echo, ${LARGE_BYTES} bytes`, async () => {
      const session = await openSession(ECHO_SOCKET);
      await session.next();
      const bytes = Uint8Array.from({ length: LARGE_BYTES }, (_, index) => index % 251);
      session.socket.send(bytes);
      const echoed = await session.next();
      session.socket.close();
      const ok = echoed instanceof ArrayBuffer && sameBytes(new Uint8Array(echoed), bytes);
      return check(
        `ws binary echo, ${LARGE_BYTES} bytes`,
        ok,
        echoed instanceof ArrayBuffer ? `${echoed.byteLength} bytes` : typeof echoed,
      );
    }),
    await attempt('ws ping', async () => {
      const session = await openSession(ECHO_SOCKET);
      await session.next();
      ping(session.socket);
      const answer = await session.next();
      session.socket.close();
      return check('ws ping', answer === 'pinged', answer);
    }),
    await attempt('ws server closes with 4001', async () => {
      const session = await openSession(ECHO_SOCKET);
      await session.next();
      session.socket.send('close:4001:bye');
      const closed = await session.closed();
      return check(
        'ws server closes with 4001',
        closed.code === 4001 && closed.reason === 'bye',
        closed,
      );
    }),
    await attempt('ws client close(1000)', async () => {
      const session = await openSession(ECHO_SOCKET);
      await session.next();
      session.socket.close(1000, 'done');
      const closed = await session.closed();
      return check('ws client close(1000)', closed.code === 1000, closed);
    }),
    await attempt('ws binary message as Blob', async () => {
      const session = await openSession(ECHO_SOCKET, undefined, {}, 'blob');
      await session.next();
      session.socket.send(new Uint8Array([1, 2, 3, 250]));
      const echoed = await session.next();
      session.socket.close();
      if (!(echoed instanceof Blob))
        return check('ws binary message as Blob', false, typeof echoed);
      const url = await readBlob(echoed, 'dataURL');
      return check(
        'ws binary message as Blob',
        echoed.size === 4 && url.endsWith(';base64,AQID+g=='),
        url,
      );
    }),
    await attempt('ws send(Blob)', async () => {
      const session = await openSession(ECHO_SOCKET);
      await session.next();
      session.socket.send(new Blob(['blob over a socket']));
      const echoed = await session.next();
      session.socket.close();
      const text =
        echoed instanceof ArrayBuffer
          ? String.fromCodePoint(...new Uint8Array(echoed))
          : String(echoed);
      return check('ws send(Blob)', text === 'blob over a socket', text);
    }),
    await attempt('wss echo (public, TLS)', async () => {
      const session = await openSession(PUBLIC_WSS_ECHO);
      session.socket.send('tizen over wss');
      const echoed = await session.next();
      session.socket.close();
      return check('wss echo (public, TLS)', echoed === 'tizen over wss', echoed);
    }),
    await attempt('ws refused connection fails', async () => {
      const outcome = await within(
        'refused',
        openSession(ECHO_SOCKET.replace(/:\d+\//, ':1/')).then(
          () => 'opened',
          (error: unknown) => (error instanceof Error ? error.message : String(error)),
        ),
      );
      return check('ws refused connection fails', outcome.includes('1006'), outcome);
    }),
  ];
}

export function WebSocketScreen() {
  return <CheckList checks={useChecks('websocket', webSocketChecks, 'modules')} />;
}
