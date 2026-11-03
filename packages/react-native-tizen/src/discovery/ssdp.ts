import { createSocket } from 'node:dgram';

/** One answer to an SSDP search: who sent it and where its UPnP description is. */
export interface SsdpReply {
  readonly host: string;
  readonly location: string;
  readonly server: string;
}

// oxlint-disable-next-line sonarjs/no-hardcoded-ip
const GROUP = '239.255.255.250'; // NOSONAR: SSDP's multicast group, fixed by UPnP Device Architecture 1.1
const PORT = 1900;
const MAX_WAIT_SECONDS = 2;
/** DIAL's service type, which a Samsung TV answers from its `nservice` description. */
export const DIAL_SEARCH_TARGET = 'urn:dial-multiscreen-org:service:dial:1';

/** The search request of UPnP Device Architecture 1.1, section 1.3.2. */
export function searchRequest(target: string): Buffer {
  return Buffer.from(
    `M-SEARCH * HTTP/1.1\r\nHOST: ${GROUP}:${PORT}\r\nMAN: "ssdp:discover"\r\nMX: ${MAX_WAIT_SECONDS}\r\nST: ${target}\r\n\r\n`,
  );
}

/** Reads a search response's headers; null without a LOCATION. */
export function parseSsdpReply(message: string, host: string): SsdpReply | null {
  const headers = new Map<string, string>();
  for (const line of message.slice(0, 8192).split(/\r?\n/).slice(1)) {
    const separator = line.indexOf(':');
    if (separator > 0)
      headers.set(line.slice(0, separator).trim().toLowerCase(), line.slice(separator + 1).trim());
  }
  const location = headers.get('location');
  if (location === undefined || !/^https?:\/\//.test(location)) return null;
  return { host, location, server: headers.get('server') ?? '' };
}

/** Multicasts one search and collects the replies until `windowMs` ends. Empty when the link is down. */
export async function ssdpSearch(
  windowMs: number,
  target = DIAL_SEARCH_TARGET,
): Promise<SsdpReply[]> {
  const replies = new Map<string, SsdpReply>();
  const socket = createSocket({ type: 'udp4', reuseAddr: true });
  let closed = false;
  const close = () => {
    if (!closed) socket.close();
    closed = true;
  };
  socket.on('error', close);
  socket.on('message', (message, from) => {
    const reply = parseSsdpReply(message.toString('utf8'), from.address);
    if (reply !== null) replies.set(`${reply.host} ${reply.location}`, reply);
  });
  await new Promise<void>((resolve) => socket.bind(resolve));
  if (!closed) socket.send(searchRequest(target), PORT, GROUP, () => undefined);
  await new Promise((resolve) => setTimeout(resolve, windowMs));
  close();
  return [...replies.values()];
}
