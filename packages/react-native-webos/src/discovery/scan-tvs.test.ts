import { createServer, type Server } from 'node:http';
import { afterEach, describe, expect, it, vi } from 'vitest';
import { arpHosts } from './arp.ts';
import { localSubnets, subnetHosts } from './lan.ts';
import { scanForTvs, sweepTargets, type ScanPorts } from './scan-tvs.ts';
import { ssdpSearch } from './ssdp.ts';

vi.mock('./arp.ts', () => ({ arpHosts: vi.fn() }));
vi.mock('./ssdp.ts', async (importOriginal) => ({
  ...(await importOriginal<typeof import('./ssdp.ts')>()),
  ssdpSearch: vi.fn(),
}));
vi.mock('./lan.ts', async (importOriginal) => ({
  ...(await importOriginal<typeof import('./lan.ts')>()),
  localSubnets: vi.fn(),
  subnetHosts: vi.fn(),
}));

const servers: Server[] = [];
const SECOND_SCREEN = 'urn:lge-com:service:webos-second-screen:1';
const UUID = '6f2c1c1e-8b0a-4c57-9d0e-2a7b3a4f5e61';

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
const CLOSED: ScanPorts = {
  remote: { secure: CLOSED_PORT, plain: CLOSED_PORT },
  ssh: CLOSED_PORT,
  keyServer: CLOSED_PORT,
};

const description = (manufacturer: string) =>
  `<root><device><friendlyName>[LG] webOS TV OLED65B4PUA</friendlyName><manufacturer>${manufacturer}</manufacturer><modelName>LG Smart TV</modelName><modelNumber>OLED65B4PUA</modelNumber><serviceList><service><serviceType>${SECOND_SCREEN}</serviceType></service></serviceList></device></root>`;

const reply = (host: string, port: number, target = SECOND_SCREEN) => ({
  host,
  location: `http://${host}:${port}/`,
  server: 'WebOS/4.1.0 UPnP/1.0',
  target,
  uuid: UUID,
});

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
  it('names a TV by its UPnP description when nothing else of it answers', async () => {
    const port = await listen((url) => (url === '/' ? description('LG Electronics') : undefined));
    vi.mocked(localSubnets).mockReturnValue([
      { iface: 'en0', address: '127.0.0.5', prefix: '127.0.0' },
    ]);
    vi.mocked(subnetHosts).mockReturnValue([]);
    vi.mocked(arpHosts).mockResolvedValue([]);
    vi.mocked(ssdpSearch).mockResolvedValue([
      reply('127.0.0.1', port),
      reply('127.0.0.9', port, 'urn:dial-multiscreen-org:service:dial:1'),
    ]);

    const found = await scanForTvs({ ports: CLOSED });

    expect(found).toEqual([
      {
        host: '127.0.0.1',
        name: '[LG] webOS TV OLED65B4PUA',
        model: 'OLED65B4PUA',
        id: UUID,
        webosVersion: null,
        developerMode: false,
        keyServer: false,
        sshPort: CLOSED_PORT,
      },
    ]);
  });

  it('skips a device that is not LG, and replies from hosts it was not asked about', async () => {
    const port = await listen(() => description('Acme'));
    vi.mocked(ssdpSearch).mockResolvedValue([reply('127.0.0.1', port), reply('127.0.0.8', port)]);

    const found = await scanForTvs({ hosts: ['127.0.0.1'], ports: CLOSED });

    expect(found).toEqual([]);
  });
});
