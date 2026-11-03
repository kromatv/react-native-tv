import { connect } from 'node:net';
import { FixableError } from '../errors.ts';
import { splitFrames, type StreamFrame } from '../logs/frame-splitter.ts';
import {
  parseCaptureReply,
  relayRequest,
  replyPayloadLength,
  type ScreenFrame,
} from './capture-protocol.ts';

const MAX_LINE_BYTES = 64 * 1024;
const LOGS_HINT = "The terminal running rn-tizen run or rn-tizen logs prints the app's logs.";

function silentListener(port: number): FixableError {
  return new FixableError(
    `The program listening on port ${port} did not answer the screenshot request`,
    'Stop it, or update it if it is an older rn-tizen, then run the command again.',
  );
}

function frameOf(frame: StreamFrame, port: number): ScreenFrame {
  const reply = parseCaptureReply(frame.line);
  if (reply.kind === 'failure') throw new FixableError(reply.message, reply.hint ?? LOGS_HINT);
  if (reply.kind === 'invalid') throw silentListener(port);
  const { width, height, stride } = reply.header;
  return { width, height, stride, pixels: frame.payload ?? Buffer.alloc(0) };
}

/**
 * Asks the `rn-tizen run` or `rn-tizen logs` that holds `port` on this
 * computer to capture the screen of the app on `device` (any when null).
 */
export function captureThroughRelay(
  port: number,
  device: string | null,
  timeoutMs: number,
): Promise<ScreenFrame> {
  return new Promise((resolve, reject) => {
    const socket = connect(port, '127.0.0.1');
    let settled = false;
    const settle = (outcome: () => void) => {
      if (settled) return;
      settled = true;
      clearTimeout(timer);
      socket.destroy();
      outcome();
    };
    const timer = setTimeout(() => settle(() => reject(silentListener(port))), timeoutMs);
    const sink = splitFrames({
      maxLineBytes: MAX_LINE_BYTES,
      payloadLength: replyPayloadLength,
      onFrame: (frame) =>
        settle(() => {
          try {
            resolve(frameOf(frame, port));
          } catch (error) {
            reject(error);
          }
        }),
    });
    socket.on('connect', () => socket.write(relayRequest(device)));
    socket.on('data', (chunk: Buffer) => sink.push(chunk));
    socket.on('end', () => sink.end());
    socket.on('error', (error) =>
      settle(() =>
        reject(
          new FixableError(
            `Cannot reach port ${port} on this computer: ${error.message}`,
            'Is rn-tizen run or rn-tizen logs still running?',
          ),
        ),
      ),
    );
    socket.on('close', () => settle(() => reject(silentListener(port))));
  });
}
