import { SDB_PORT, type SdbAddress } from '../sdb/sdb-connection.ts';

const ADDRESS = /^(\d{1,3}(?:\.\d{1,3}){3})(?::(\d{1,5}))?$/;

/** A TV address as typed: `192.168.1.50`, or `192.168.1.50:26101` with a port. */
export function parseDeviceAddress(text: string): SdbAddress | null {
  const match = ADDRESS.exec(text.trim());
  if (match?.[1] === undefined) return null;
  if (match[1].split('.').some((part) => Number(part) > 255)) return null;
  const port = match[2] === undefined ? SDB_PORT : Number(match[2]);
  return port >= 1 && port <= 65535 ? { host: match[1], port } : null;
}

/** `host:port`, the name sdb gives a network device. */
export function deviceSerial({ host, port }: SdbAddress): string {
  return `${host}:${port}`;
}
