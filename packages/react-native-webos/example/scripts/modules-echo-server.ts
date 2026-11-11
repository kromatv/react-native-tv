/**
 * The server the `src/screens/modules` checks talk to: run it on the machine
 * the TV or the simulator reaches (`bun scripts/modules-echo-server.ts`).
 *
 * - `/socket` upgrades to a WebSocket that greets with the request headers,
 *   echoes every message, answers a ping with `pinged`, and closes with
 *   `code` and `reason` on `close:<code>:<reason>`.
 * - `/utf8` answers UTF-8 text, `/echo-body` answers the request body, `/slow`
 *   answers after five seconds.
 */
const PORT = Number(process.env['MODULES_ECHO_PORT'] ?? 47100);
const UTF8_TEXT = 'héllo wörld ✓';

interface SocketData {
  readonly headers: Record<string, string>;
}

const server = Bun.serve<SocketData>({
  port: PORT,
  hostname: '0.0.0.0',
  async fetch(request, server) {
    const url = new URL(request.url);
    if (url.pathname === '/utf8') {
      return new Response(UTF8_TEXT, { headers: { 'content-type': 'text/plain; charset=utf-8' } });
    }
    if (url.pathname === '/slow') {
      await Bun.sleep(5000);
      return new Response('slow');
    }
    if (url.pathname === '/echo-body') {
      return new Response(await request.arrayBuffer(), {
        headers: {
          'content-type': request.headers.get('content-type') ?? 'application/octet-stream',
        },
      });
    }
    if (url.pathname === '/socket') {
      const protocol = (request.headers.get('sec-websocket-protocol') ?? '').split(',')[0]?.trim();
      const upgraded = server.upgrade(request, {
        data: { headers: Object.fromEntries(request.headers) },
        headers: protocol ? { 'Sec-WebSocket-Protocol': protocol } : undefined,
      });
      return upgraded ? undefined : new Response('Expected a WebSocket upgrade', { status: 400 });
    }
    return new Response('Not found', { status: 404 });
  },
  websocket: {
    open(socket) {
      socket.send(JSON.stringify({ type: 'hello', headers: socket.data.headers }));
    },
    message(socket, message) {
      if (typeof message === 'string' && message.startsWith('close:')) {
        const [, code, reason] = message.split(':');
        socket.close(Number(code), reason);
        return;
      }
      socket.send(message);
    },
    ping(socket) {
      socket.send('pinged');
    },
  },
});

console.log(`modules echo server on http://0.0.0.0:${server.port}`);
