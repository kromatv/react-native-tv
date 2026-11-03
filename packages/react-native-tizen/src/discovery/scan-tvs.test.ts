import { createServer, type Server } from 'node:http';
import { afterEach, describe, expect, it, vi } from 'vitest';
import { arpHosts } from './arp.ts';
import { localSubnets, subnetHosts } from './lan.ts';
import { scanForTvs, sweepTargets } from './scan-tvs.ts';
import { ssdpSearch } from './ssdp.ts';

vi.mock('./arp.ts', () => ({ arpHosts: vi.fn() }));
vi.mock('./ssdp.ts', () => ({ ssdpSearch: vi.fn() }));
vi.mock('./lan.ts', async (importOriginal) => ({
  ...(await importOriginal<typeof import('./lan.ts')>()),
  localSubnets: vi.fn(),
  subnetHosts: vi.fn(),
}));

const servers: Server[] = [];

async function listen(handler: (url: string) => string | undefined): Promise<number> {
  const server = createServer((request, response) => {
    const body = handler(request.url ?? '');
    response.statusCode = body === undefined ? 404 : 200;
    response.end(body ?? '');
  });
  servers.push(server);
  await new Promise<void>((resolve) => server.listen(0, '127.0.0.1', resolve));
  const address = server.address();
  if (address === null || typeof address === 'string') throw new Error('the server got no port');
  return address.port;
}

/** No socket ever listens on port 0: binding to it asks the OS for any free port. */
const CLOSED_PORT = 0;

const description = (manufacturer: string) =>
  `<root><device><friendlyName>Bedroom</friendlyName><manufacturer>${manufacturer}</manufacturer><modelName>UE43</modelName></device></root>`;

afterEach(async () => {
  vi.resetAllMocks();
  await Promise.all(
    servers.splice(0).map((server) => new Promise((resolve) => server.close(resolve))),
  );
});

describe('sweepTargets', () => {
  it('puts the hosts this machine talked to lately first, on its own networks only', async () => {
    vi.mocked(localSubnets).mockReturnValue([
      { iface: 'en0', address: '10.1.1.5', prefix: '10.1.1' },
    ]);
    vi.mocked(subnetHosts).mockReturnValue(['10.1.1.1', '10.1.1.7']);
    vi.mocked(arpHosts).mockResolvedValue(['10.1.1.7', '192.168.9.9', 'not-an-address']);

    expect(await sweepTargets()).toEqual(['10.1.1.7', '10.1.1.1']);
  });
});

describe('scanForTvs over SSDP', () => {
  it('names a TV by its UPnP description when its REST API is closed', async () => {
    const port = await listen((url) =>
      url === '/dial.xml' ? description('Samsung Electronics') : undefined,
    );
    vi.mocked(localSubnets).mockReturnValue([
      { iface: 'en0', address: '127.0.0.5', prefix: '127.0.0' },
    ]);
    vi.mocked(subnetHosts).mockReturnValue([]);
    vi.mocked(arpHosts).mockResolvedValue([]);
    vi.mocked(ssdpSearch).mockResolvedValue([
      {
        host: '127.0.0.1',
        location: `http://127.0.0.1:${port}/dial.xml`,
        server: 'Samsung/1.0 UPnP/1.0',
      },
      { host: '127.0.0.9', location: `http://127.0.0.9:${port}/dial.xml`, server: 'Roku/9.0' },
    ]);

    const found = await scanForTvs({ ports: { api: CLOSED_PORT, sdb: CLOSED_PORT } });

    expect(found).toMatchObject([
      { host: '127.0.0.1', name: 'Bedroom', model: 'UE43', developerMode: null },
    ]);
  });

  it('skips a device that is not Samsung, and replies from hosts it was not asked about', async () => {
    const port = await listen(() => description('Acme'));
    vi.mocked(ssdpSearch).mockResolvedValue([
      { host: '127.0.0.1', location: `http://127.0.0.1:${port}/dial.xml`, server: 'Samsung/1.0' },
      { host: '127.0.0.8', location: `http://127.0.0.8:${port}/dial.xml`, server: 'Samsung/1.0' },
    ]);

    const found = await scanForTvs({
      hosts: ['127.0.0.1'],
      ports: { api: CLOSED_PORT, sdb: CLOSED_PORT },
    });

    expect(found).toEqual([]);
  });
});
