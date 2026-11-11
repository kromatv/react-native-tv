/** The port of the SSH server a TV runs while Developer Mode is on. */
export const DEVELOPER_SSH_PORT = 9922;

/** Where a TV's Developer Mode SSH server listens. */
export interface TvAddress {
  readonly host: string;
  readonly port: number;
}

const ADDRESS = /^(\d{1,3}(?:\.\d{1,3}){3})(?::(\d{1,5}))?$/;

/** A TV address as typed: `192.168.1.50`, or `192.168.1.50:9922` with a port. */
export function parseDeviceAddress(text: string): TvAddress | null {
  const match = ADDRESS.exec(text.trim());
  if (match?.[1] === undefined) return null;
  if (match[1].split('.').some((part) => Number(part) > 255)) return null;
  const port = match[2] === undefined ? DEVELOPER_SSH_PORT : Number(match[2]);
  return port >= 1 && port <= 65535 ? { host: match[1], port } : null;
}

/** The address as `--device` takes it: the host, with the port only when it is not 9922. */
export function addressText({ host, port }: TvAddress): string {
  return port === DEVELOPER_SSH_PORT ? host : `${host}:${port}`;
}
