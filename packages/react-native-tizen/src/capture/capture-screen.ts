import {
  CAPTURE_TIMEOUT_MS,
  LINK_WAIT_MS,
  listenForLogs,
  PortInUseError,
} from '../logs/log-server.ts';
import type { ScreenFrame } from './capture-protocol.ts';
import { captureThroughRelay } from './relay.ts';

const RELAY_SLACK_MS = 5_000;

/**
 * Captures the screen of the development build on `device` (the only one
 * connected when null) that streams its logs to `port`: through the
 * `rn-tizen run` or `logs` holding that port, else by listening on it until
 * the app's control link connects. Log lines that arrive meanwhile are dropped.
 */
export async function captureScreen(port: number, device: string | null): Promise<ScreenFrame> {
  const listener = { onEntry: () => undefined };
  const server = await listenForLogs(port, listener, { peer: device }).catch((error: unknown) => {
    if (error instanceof PortInUseError) return null;
    throw error;
  });
  if (server === null) {
    return captureThroughRelay(port, device, LINK_WAIT_MS + CAPTURE_TIMEOUT_MS + RELAY_SLACK_MS);
  }
  try {
    return await server.capture(device, LINK_WAIT_MS);
  } finally {
    await server.close();
  }
}
