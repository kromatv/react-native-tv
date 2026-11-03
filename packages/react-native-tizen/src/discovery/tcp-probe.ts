import { createConnection, type Socket } from 'node:net';

type Connect = (options: { host: string; port: number }) => Socket;

/** Whether something accepts a TCP connection at `host:port` within `timeoutMs`. */
export function tcpOpen(
  host: string,
  port: number,
  timeoutMs: number,
  connect: Connect = createConnection,
): Promise<boolean> {
  return new Promise((resolve) => {
    const socket = connect({ host, port });
    const settle = (open: boolean) => {
      socket.destroy();
      resolve(open);
    };
    socket.setTimeout(timeoutMs);
    socket.once('connect', () => settle(true));
    socket.once('timeout', () => settle(false));
    socket.once('error', () => settle(false));
  });
}
