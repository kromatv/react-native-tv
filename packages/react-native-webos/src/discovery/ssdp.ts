import { createSocket } from 'node:dgram';
import { z } from 'zod';

/** One answer to an SSDP search: who sent it, what it answered for and where its UPnP description is. */
export interface SsdpReply {
  readonly host: string;
  readonly location: string;
  readonly server: string;
  /** The search target the device answered, its `ST` header. */
  readonly target: string;
  /** The device's UUID, cut out of `USN`; null when it has none. */
  readonly uuid: string | null;
}

// oxlint-disable-next-line sonarjs/no-hardcoded-ip
const GROUP = '239.255.255.250'; // NOSONAR: SSDP's multicast group, fixed by UPnP Device Architecture 1.1
const PORT = 1900;
const MAX_WAIT_SECONDS = 2;
const MAX_REPLY_CHARS = 8192;
const USN_UUID = /^uuid:([0-9A-Za-z-]{1,64})(?:::|$)/;
const headersSchema = z.object({
  location: z.url({ protocol: /^https?$/ }).max(2048),
  server: z.string().max(256).default(''),
  st: z.string().max(256).default(''),
  usn: z.string().max(512).default(''),
});
/** The service of an LG TV's remote control, which only its Basic device answers a search for. */
export const SECOND_SCREEN_SEARCH_TARGET = 'urn:lge-com:service:webos-second-screen:1';

/** The search request of UPnP Device Architecture 1.1, section 1.3.2. */
export function searchRequest(target: string): Buffer {
  return Buffer.from(
    `M-SEARCH * HTTP/1.1\r\nHOST: ${GROUP}:${PORT}\r\nMAN: "ssdp:discover"\r\nMX: ${MAX_WAIT_SECONDS}\r\nST: ${target}\r\n\r\n`,
  );
}

/** Reads a search response's headers; null without an HTTP LOCATION. */
export function parseSsdpReply(message: string, host: string): SsdpReply | null {
  const headers: Record<string, string> = {};
  for (const line of message.slice(0, MAX_REPLY_CHARS).split(/\r?\n/).slice(1)) {
    const separator = line.indexOf(':');
    if (separator > 0)
      headers[line.slice(0, separator).trim().toLowerCase()] = line.slice(separator + 1).trim();
  }
  const parsed = headersSchema.safeParse(headers);
  if (!parsed.success) return null;
  const { location, server, st, usn } = parsed.data;
  return { host, location, server, target: st, uuid: USN_UUID.exec(usn)?.[1] ?? null };
}

/**
 * Multicasts a search, once more halfway through, and collects the replies
 * until `windowMs` ends. Empty when the link is down.
 */
export async function ssdpSearch(
  windowMs: number,
  target = SECOND_SCREEN_SEARCH_TARGET,
): Promise<SsdpReply[]> {
  const replies = new Map<string, SsdpReply>();
  const socket = createSocket({ type: 'udp4', reuseAddr: true });
  let closed = false;
  const close = () => {
    if (!closed) socket.close();
    closed = true;
  };
  const search = () => {
    if (!closed) socket.send(searchRequest(target), PORT, GROUP, () => undefined);
  };
  socket.on('error', close);
  socket.on('message', (message, from) => {
    const reply = parseSsdpReply(message.toString('utf8'), from.address);
    if (reply !== null) replies.set(`${reply.host} ${reply.location}`, reply);
  });
  await new Promise<void>((resolve) => socket.bind(resolve));
  search();
  await new Promise((resolve) => setTimeout(resolve, windowMs / 2));
  search();
  await new Promise((resolve) => setTimeout(resolve, windowMs / 2));
  close();
  return [...replies.values()];
}
