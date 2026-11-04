import type { Socket } from 'node:net';
import { FixableError } from '../errors.ts';
import type { StreamFrame } from '../logs/frame-splitter.ts';
import {
  parseCaptureReply,
  perfRequest,
  screenshotRequest,
  type ScreenFrame,
} from './capture-protocol.ts';

const LOGS_HINT = 'Its logs say why: rn-tizen logs --level debug.';
const VERSION_HINT = 'Build the app again with this version of rn-tizen.';

interface PendingCapture {
  readonly resolve: (frame: ScreenFrame) => void;
  readonly reject: (error: Error) => void;
  readonly timer: NodeJS.Timeout;
}

/**
 * The control connection a development build keeps open to `rn-tizen`, beside
 * its log stream: `rn-tizen` asks, the app answers with its screen, or turns
 * its perf samples on.
 */
export class ControlLink {
  readonly address: string;
  /** The package the app named in its hello, null for a build that names none. */
  readonly app: string | null;
  private readonly socket: Socket;
  private readonly pending = new Map<number, PendingCapture>();
  private nextId = 1;

  constructor(socket: Socket, address: string, app: string | null = null) {
    this.socket = socket;
    this.address = address;
    this.app = app;
  }

  /** Asks the app for its screen; rejects after `timeoutMs` or when the link drops. */
  capture(timeoutMs: number): Promise<ScreenFrame> {
    const id = this.nextId;
    this.nextId += 1;
    return new Promise((resolve, reject) => {
      const timer = setTimeout(() => {
        this.pending.delete(id);
        reject(
          new FixableError(
            `The app at ${this.address} sent no screenshot within ${timeoutMs / 1000} s`,
            LOGS_HINT,
          ),
        );
      }, timeoutMs);
      this.pending.set(id, { resolve, reject, timer });
      this.socket.write(screenshotRequest(id));
    });
  }

  /** Asks the app to log a `perf` sample every second, or to stop. It stops on its own once the link drops. */
  requestPerf(on: boolean): void {
    this.socket.write(perfRequest(on));
  }

  /** Takes one frame the app sent on this link. */
  receive(frame: StreamFrame): void {
    const reply = parseCaptureReply(frame.line);
    if (reply.kind === 'invalid') {
      this.fail(
        new FixableError(
          `The app at ${this.address} sent a bad reply: ${reply.reason}`,
          VERSION_HINT,
        ),
      );
      this.socket.destroy();
      return;
    }
    const id = reply.kind === 'frame' ? reply.header.id : reply.id;
    const waiting = this.pending.get(id);
    if (waiting === undefined) return;
    this.pending.delete(id);
    clearTimeout(waiting.timer);
    if (reply.kind === 'failure') {
      waiting.reject(
        new FixableError(
          `The app could not capture its screen: ${reply.message}`,
          reply.hint ?? LOGS_HINT,
        ),
      );
      return;
    }
    const { width, height, stride } = reply.header;
    waiting.resolve({ width, height, stride, pixels: frame.payload ?? Buffer.alloc(0) });
  }

  /** Rejects every capture still waiting, as the link is gone. */
  closed(): void {
    this.fail(
      new FixableError(`The app at ${this.address} disconnected before it answered`, LOGS_HINT),
    );
  }

  private fail(error: Error): void {
    for (const waiting of this.pending.values()) {
      clearTimeout(waiting.timer);
      waiting.reject(error);
    }
    this.pending.clear();
  }
}
