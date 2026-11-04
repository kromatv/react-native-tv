import { createServer, type Socket } from 'node:net';
import {
  encodeFailure,
  encodeFrame,
  parseHello,
  parseRelayRequest,
  replyPayloadLength,
  type ScreenFrame,
} from '../capture/capture-protocol.ts';
import { ControlLink } from '../capture/control-link.ts';
import { ControlLinks } from '../capture/control-links.ts';
import { FixableError } from '../errors.ts';
import { splitFrames, type StreamFrame } from './frame-splitter.ts';
import { parseLogLine, type LogEntry } from './log-line.ts';

const MAX_LINE_BYTES = 64 * 1024;
const LOOPBACK = new Set(['127.0.0.1', '::1']);

/** How long a relayed screenshot waits for the app's control link to connect. */
export const LINK_WAIT_MS = 10_000;

/** How long an app may take to send its screen once asked. */
export const CAPTURE_TIMEOUT_MS = 30_000;

/** What {@link listenForLogs} tells its caller about. `app` is the package a build named, null for one that names none. */
export interface LogListener {
  /** Asks each app's control link for `perf` samples, which then arrive as entries. */
  readonly wantsPerf?: boolean;
  onEntry(entry: LogEntry, from: string, app: string | null): void;
  onConnect?(from: string, app: string | null): void;
  onDisconnect?(from: string, app: string | null): void;
}

/** A running log listener, which also reaches the apps that stream to it. */
export interface LogServer {
  readonly port: number;
  /**
   * Asks the app `app` on `device`, either the only one connected when null,
   * for its screen, waiting up to `waitMs` for its control link to connect.
   */
  capture(device: string | null, app: string | null, waitMs: number): Promise<ScreenFrame>;
  close(): Promise<void>;
}

/** Who may connect to {@link listenForLogs}, and what to say when its port is taken. */
export interface ListenOptions {
  readonly inUseHint?: string;
  /** The TV's address, when known: a connection from any other host but this computer is dropped. */
  readonly peer?: string | null;
  /** The package to listen to: a build that names another one is ignored. */
  readonly app?: string | null;
}

/** The port {@link listenForLogs} was asked for is held by another program. */
export class PortInUseError extends FixableError {}

type Role =
  | { readonly kind: 'pending' }
  | { readonly kind: 'log'; readonly app: string | null }
  | { readonly kind: 'control'; readonly link: ControlLink }
  | { readonly kind: 'relay' }
  | { readonly kind: 'ignored' };

const PORT_IN_USE_HINT =
  'Another `rn-tizen logs` or `rn-tizen run` is probably listening: stop it, or pass --log-port to package and run on another port.';

function addressOf(socket: Socket): string {
  return socket.remoteAddress?.replace(/^::ffff:/, '') ?? 'the TV';
}

/** True when a connection from `from` may stay: this computer, or `peer` when one is known. */
export function acceptsPeer(from: string, peer: string | null): boolean {
  return peer === null || from === peer || LOOPBACK.has(from);
}

function noLinkError(device: string | null, app: string | null, port: number): FixableError {
  const build = app === null ? 'No development build' : `No development build of ${app}`;
  const where = device === null ? build : `${build} on ${device}`;
  return new FixableError(
    `${where} is connected to port ${port}`,
    `Launch it with rn-tizen run, or from the TV's apps, packaged with this port. A release build has no screenshots.`,
  );
}

function failureLine(error: unknown): string {
  if (error instanceof FixableError) return encodeFailure(0, error.message, error.hint);
  return encodeFailure(0, error instanceof Error ? error.message : String(error));
}

/**
 * Listens for what a development build streams over TCP: its JSON log lines,
 * from any number of app launches, and its control link, until closed. A
 * process on this computer may also ask it, on the same port, to capture an
 * app's screen.
 */
export function listenForLogs(
  port: number,
  listener: LogListener,
  options: ListenOptions = {},
): Promise<LogServer> {
  const inUseHint = options.inUseHint ?? PORT_IN_USE_HINT;
  const peer = options.peer ?? null;
  const target = options.app ?? null;
  const wanted = (app: string | null) => target === null || app === null || app === target;
  const sockets = new Set<Socket>();
  const links = new ControlLinks();
  let boundPort = port;

  const capture = async (device: string | null, app: string | null, waitMs: number) => {
    const link = await links.find(device, app, waitMs);
    if (link === null) throw noLinkError(device, app, boundPort);
    return link.capture(CAPTURE_TIMEOUT_MS);
  };

  const relay = (socket: Socket, device: string | null, app: string | null) => {
    capture(device, app, LINK_WAIT_MS).then(
      (frame) => socket.end(encodeFrame(0, frame)),
      (error: unknown) => socket.end(failureLine(error)),
    );
  };

  const classify = (socket: Socket, from: string, line: string): Role => {
    const hello = parseHello(line);
    if (hello !== null && !wanted(hello.app)) return { kind: 'ignored' };
    if (hello?.kind === 'control') {
      const link = new ControlLink(socket, from, hello.app);
      links.add(link);
      if (listener.wantsPerf === true) link.requestPerf(true);
      return { kind: 'control', link };
    }
    const request = parseRelayRequest(line);
    if (request !== undefined && LOOPBACK.has(from)) {
      relay(socket, request.device, request.app ?? target);
      return { kind: 'relay' };
    }
    const app = hello?.app ?? null;
    listener.onConnect?.(from, app);
    if (hello === null) listener.onEntry(parseLogLine(line), from, app);
    return { kind: 'log', app };
  };

  const server = createServer((socket) => {
    const from = addressOf(socket);
    if (!acceptsPeer(from, peer)) {
      socket.destroy();
      return;
    }
    let role: Role = { kind: 'pending' };
    sockets.add(socket);
    const onFrame = (frame: StreamFrame) => {
      if (role.kind === 'pending') role = classify(socket, from, frame.line);
      else if (role.kind === 'log') listener.onEntry(parseLogLine(frame.line), from, role.app);
      else if (role.kind === 'control') role.link.receive(frame);
    };
    const sink = splitFrames({
      maxLineBytes: MAX_LINE_BYTES,
      payloadLength: (line) => (role.kind === 'control' ? replyPayloadLength(line) : 0),
      onFrame,
    });
    socket.on('data', (chunk: Buffer) => sink.push(chunk));
    socket.on('end', () => sink.end());
    socket.on('error', () => socket.destroy());
    socket.on('close', () => {
      sockets.delete(socket);
      if (role.kind === 'control') {
        links.remove(role.link);
        role.link.closed();
      }
      if (role.kind === 'log') listener.onDisconnect?.(from, role.app);
    });
  });

  const close = () =>
    new Promise<void>((done) => {
      for (const socket of sockets) socket.destroy();
      server.close(() => done());
    });

  return new Promise((resolve, reject) => {
    server.once('error', (error: NodeJS.ErrnoException) => {
      if (error.code !== 'EADDRINUSE') return reject(error);
      reject(new PortInUseError(`Port ${port} is already in use`, inUseHint));
    });
    server.listen(port, () => {
      const address = server.address();
      boundPort = typeof address === 'object' && address !== null ? address.port : port;
      resolve({ port: boundPort, capture, close });
    });
  });
}
