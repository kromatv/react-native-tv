import { createServer, type Server } from 'node:http';
import { Socket } from 'node:net';
import { afterEach, describe, expect, it, vi } from 'vitest';
import { fakeSsapTv, type FakeSsapTv } from '../remote/fake-ssap-tv.fixtures.ts';
import { useFakeTvs, useTemporaryState } from '../webos/connected-tv.fixtures.ts';
import { fetchText } from './http-text.ts';
import { probeTv } from './probe-tv.ts';
import { scanForTvs, type FoundTv } from './scan-tvs.ts';
import { ssdpSearch } from './ssdp.ts';
import { tcpOpen } from './tcp-probe.ts';
import { fetchTvIdentity } from './tv-identity.ts';
import { fetchDeviceDescription } from './upnp.ts';

vi.mock('./ssdp.ts', async (importOriginal) => {
  const original = await importOriginal<typeof import('./ssdp.ts')>();
  return { ...original, ssdpSearch: vi.fn(original.ssdpSearch) };
});

const servers: Server[] = [];
const remotes: FakeSsapTv[] = [];

const DESCRIPTION =
  '<root><device><friendlyName>[LG] webOS TV OLED65B4PUA</friendlyName><manufacturer>LG Electronics</manufacturer><modelNumber>OLED65B4PUA</modelNumber></device></root>';

async function http(routes: Record<string, string>): Promise<number> {
  const server = createServer((request, response) => {
    const body = routes[request.url ?? ''];
    response.statusCode = body === undefined ? 404 : 200;
    response.end(body ?? '');
  });
  servers.push(server);
  await new Promise<void>((resolve) => server.listen(0, '127.0.0.1', resolve));
  const address = server.address();
  if (address === null || typeof address === 'string') throw new Error('the server got no port');
  return address.port;
}

async function remote(): Promise<FakeSsapTv> {
  const started = await fakeSsapTv();
  remotes.push(started);
  return started;
}

/** No socket ever listens on port 0: binding to it asks the OS for any free port. */
const CLOSED_PORT = 0;

useTemporaryState();
const connect = useFakeTvs();

afterEach(async () => {
  vi.mocked(ssdpSearch).mockRestore();
  await Promise.all(
    servers.splice(0).map((server) => new Promise((resolve) => server.close(resolve))),
  );
  await Promise.all(remotes.splice(0).map((tv) => tv.close()));
});

describe('fetchText', () => {
  it('reads a body, and gives null for a refusal or a body too large', async () => {
    const port = await http({ '/ok': 'hello', '/big': 'x'.repeat(100) });
    const base = `http://127.0.0.1:${port}`;

    expect(await fetchText(`${base}/ok`)).toBe('hello');
    expect(await fetchText(`${base}/missing`)).toBeNull();
    expect(await fetchText(`${base}/big`, 10)).toBeNull();
    expect(await fetchText(`http://127.0.0.1:${CLOSED_PORT}/`)).toBeNull();
  });
});

describe('the TV answers', () => {
  it('reads the UPnP description', async () => {
    const port = await http({ '/': DESCRIPTION });

    expect(await fetchDeviceDescription(`http://127.0.0.1:${port}/`)).toMatchObject({
      friendlyName: '[LG] webOS TV OLED65B4PUA',
      modelNumber: 'OLED65B4PUA',
    });
    expect(await fetchDeviceDescription(`http://127.0.0.1:${port}/missing`)).toBeNull();
  });

  it('reads the release, the id and the model without pairing, and not the serial number', async () => {
    const tv = await remote();

    const identity = await fetchTvIdentity('127.0.0.1', { secure: CLOSED_PORT, plain: tv.port });

    expect(identity).toEqual({ uuid: tv.uuid, releaseVersion: '11.2.0', model: 'OLED65C25LB' });
    expect(tv.frames.map((frame) => frame.type)).toEqual(['hello', 'request']);
  });

  it('identifies nothing where no TV listens, or a web server does', async () => {
    const web = await http({ '/': 'Hello world' });

    expect(
      await fetchTvIdentity('127.0.0.1', { secure: CLOSED_PORT, plain: CLOSED_PORT }),
    ).toBeNull();
    expect(await fetchTvIdentity('127.0.0.1', { secure: CLOSED_PORT, plain: web })).toBeNull();
  });

  it('tells an open port from a refused one, a closed one and one that never answers', async () => {
    const refusing = () => {
      const socket = new Socket();
      process.nextTick(() =>
        socket.emit(
          'error',
          Object.assign(new Error('connect ECONNREFUSED'), { code: 'ECONNREFUSED' }),
        ),
      );
      return socket;
    };

    expect(await tcpOpen('127.0.0.1', await http({}), 500)).toBe(true);
    expect(await tcpOpen('192.0.2.1', 9922, 500, refusing)).toBe(false);
    expect(await tcpOpen('192.0.2.1', 9922, 20, () => new Socket())).toBe(false);
    expect(await tcpOpen('127.0.0.1', CLOSED_PORT, 500)).toBe(false);
  });

  it('collects no SSDP reply when nothing answers', async () => {
    expect(await ssdpSearch(50, 'urn:rn-webos-test:none')).toEqual([]);
  });
});

describe('scanForTvs', () => {
  it('finds a TV by its remote control service, and one by its Developer Mode port alone', async () => {
    vi.mocked(ssdpSearch).mockResolvedValue([]);
    const tv = await remote();
    const { tv: ssh } = await connect();
    const keyServer = await http({});
    const progress: number[] = [];

    const byRemote = await scanForTvs({
      hosts: ['127.0.0.1'],
      ports: { remote: { secure: CLOSED_PORT, plain: tv.port }, ssh: ssh.port, keyServer },
      onProgress: (done) => progress.push(done),
    });
    const bySsh = await scanForTvs({
      hosts: ['127.0.0.1'],
      ports: {
        remote: { secure: CLOSED_PORT, plain: CLOSED_PORT },
        ssh: ssh.port,
        keyServer: CLOSED_PORT,
      },
    });

    expect(byRemote).toEqual([
      {
        host: '127.0.0.1',
        name: '',
        model: 'OLED65C25LB',
        id: tv.uuid,
        webosVersion: '11.2.0',
        developerMode: true,
        keyServer: true,
        sshPort: ssh.port,
      },
    ]);
    expect(progress).toEqual([1]);
    expect(bySsh).toMatchObject([
      { host: '127.0.0.1', developerMode: true, keyServer: false, name: '', webosVersion: null },
    ]);
  });

  it('finds nothing where nothing answers', async () => {
    vi.mocked(ssdpSearch).mockResolvedValue([]);
    const found = await scanForTvs({
      hosts: ['127.0.0.1'],
      ports: {
        remote: { secure: CLOSED_PORT, plain: CLOSED_PORT },
        ssh: CLOSED_PORT,
        keyServer: CLOSED_PORT,
      },
    });

    expect(found).toEqual([]);
  });
});

describe('probeTv', () => {
  const found = (sshPort: number): FoundTv => ({
    host: '127.0.0.1',
    name: 'TV',
    model: '',
    id: null,
    webosVersion: null,
    developerMode: true,
    keyServer: false,
    sshPort,
  });

  it('logs in to a connected TV and fills in what the scan did not learn', async () => {
    const { tv } = await connect({ name: 'lounge' });

    const probe = await probeTv(found(tv.port));

    expect(probe).toMatchObject({ model: 'OLED65C25LB', webosVersion: '11.2.0' });
    expect(probe.connection).toMatchObject({ state: 'ready', name: 'lounge' });
  });

  it('tells a TV never connected from one that no longer lets rn-webos in', async () => {
    const refusing = await connect({ name: 'office', acceptKey: false });

    expect((await probeTv(found(CLOSED_PORT))).connection).toEqual({ state: 'unknown' });
    expect((await probeTv(found(refusing.tv.port))).connection).toMatchObject({
      state: 'failed',
      name: 'office',
      reason: expect.stringContaining('refused the key'),
    });
  });
});
