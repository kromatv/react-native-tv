import { createServer, type Server } from 'node:http';
import { Socket } from 'node:net';
import { afterEach, describe, expect, it } from 'vitest';
import { z } from 'zod';
import {
  capabilityPayload,
  fakeTv,
  TV_CAPABILITIES,
  type FakeTv,
} from '../sdb/fake-tv.fixtures.ts';
import { fetchJson, fetchText } from './http-text.ts';
import { probeTv } from './probe-tv.ts';
import { fetchApiDevice } from './samsung-api.ts';
import { scanForTvs } from './scan-tvs.ts';
import { ssdpSearch } from './ssdp.ts';
import { tcpOpen } from './tcp-probe.ts';
import { fetchDeviceDescription } from './upnp.ts';

const servers: Server[] = [];
const tvs: FakeTv[] = [];

const TV_API = {
  device: {
    OS: 'Tizen',
    name: 'Living room',
    modelName: 'QE55Q80',
    developerMode: '1',
    developerIP: '10.0.0.2',
  },
};
const DESCRIPTION =
  '<root><device><friendlyName>Bedroom</friendlyName><manufacturer>Samsung Electronics</manufacturer><modelName>UE43</modelName></device></root>';

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

/** No socket ever listens on port 0: binding to it asks the OS for any free port. */
const CLOSED_PORT = 0;

afterEach(async () => {
  await Promise.all(
    servers.splice(0).map((server) => new Promise((resolve) => server.close(resolve))),
  );
  await Promise.all(tvs.splice(0).map((tv) => tv.close()));
});

describe('fetchText and fetchJson', () => {
  it('read a body, and give null for a refusal, a body too large or JSON that does not fit', async () => {
    const port = await http({ '/ok': 'hello', '/json': '{"a":1}', '/big': 'x'.repeat(100) });
    const base = `http://127.0.0.1:${port}`;

    expect(await fetchText(`${base}/ok`)).toBe('hello');
    expect(await fetchText(`${base}/missing`)).toBeNull();
    expect(await fetchText(`${base}/big`, 10)).toBeNull();
    expect(await fetchJson(`${base}/json`, z.object({ a: z.number() }))).toEqual({ a: 1 });
    expect(await fetchJson(`${base}/json`, z.object({ b: z.string() }))).toBeNull();
    expect(await fetchJson(`${base}/ok`, z.unknown())).toBeNull();
    expect(await fetchText(`http://127.0.0.1:${CLOSED_PORT}/`)).toBeNull();
  });
});

describe('the TV answers', () => {
  it('reads the REST API and the UPnP description', async () => {
    const port = await http({ '/api/v2/': JSON.stringify(TV_API), '/description': DESCRIPTION });

    expect(await fetchApiDevice('127.0.0.1', port)).toMatchObject({
      name: 'Living room',
      developerMode: true,
    });
    expect(await fetchDeviceDescription(`http://127.0.0.1:${port}/description`)).toMatchObject({
      friendlyName: 'Bedroom',
    });
    expect(await fetchDeviceDescription(`http://127.0.0.1:${port}/missing`)).toBeNull();
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
    expect(await tcpOpen('192.0.2.1', 26101, 500, refusing)).toBe(false);
    expect(await tcpOpen('192.0.2.1', 26101, 20, () => new Socket())).toBe(false);
    expect(await tcpOpen('127.0.0.1', CLOSED_PORT, 500)).toBe(false);
  });

  it('collects no SSDP reply when nothing answers', async () => {
    expect(await ssdpSearch(50, 'urn:rn-tizen-test:none')).toEqual([]);
  });
});

describe('scanForTvs', () => {
  it('finds a TV by its REST API, and one by its sdb port alone', async () => {
    const api = await http({ '/api/v2/': JSON.stringify(TV_API) });
    const tv = await fakeTv({ services: {} });
    tvs.push(tv);
    const progress: number[] = [];

    const byApi = await scanForTvs({
      hosts: ['127.0.0.1'],
      ports: { api, sdb: tv.port },
      ssdpWindowMs: 50,
      onProgress: (done) => progress.push(done),
    });
    const bySdb = await scanForTvs({
      hosts: ['127.0.0.1'],
      ports: { api: CLOSED_PORT, sdb: tv.port },
      ssdpWindowMs: 50,
    });

    expect(byApi).toEqual([
      {
        host: '127.0.0.1',
        name: 'Living room',
        model: 'QE55Q80',
        developerMode: true,
        developerIp: '10.0.0.2',
        sdbPort: tv.port,
      },
    ]);
    expect(progress).toEqual([1]);
    expect(bySdb).toMatchObject([{ host: '127.0.0.1', developerMode: true, name: '' }]);
  });

  it('finds nothing where nothing answers', async () => {
    const found = await scanForTvs({
      hosts: ['127.0.0.1'],
      ports: { api: CLOSED_PORT, sdb: CLOSED_PORT },
      ssdpWindowMs: 50,
    });

    expect(found).toEqual([]);
  });
});

describe('probeTv', () => {
  const found = (sdbPort: number) => ({
    host: '127.0.0.1',
    name: 'TV',
    model: '',
    developerMode: true,
    developerIp: '127.0.0.1',
    sdbPort,
  });

  it('reads the capabilities, the model and the DUID of a TV whose sdb answers', async () => {
    const tv = await fakeTv({
      services: {
        'capability:': (stream) => {
          stream.write(capabilityPayload(TV_CAPABILITIES));
          stream.close();
        },
        'shell:0 getduid': (stream) => {
          stream.write('ABCDEFGH12345\r\n');
          stream.close();
        },
      },
    });
    tvs.push(tv);

    const probe = await probeTv(found(tv.port));

    expect(probe).toMatchObject({ model: 'GQ75LS03DAUXZG', hostPcIsThisComputer: true });
    expect(probe.sdb).toMatchObject({ state: 'ready', duid: 'ABCDEFGH12345' });
  });

  it('tells a TV held by another host from one whose sdb is closed', async () => {
    const busy = await fakeTv({ services: {}, handshake: 'reset' });
    tvs.push(busy);

    expect((await probeTv(found(busy.port))).sdb.state).toBe('refused');
    expect((await probeTv(found(CLOSED_PORT))).sdb.state).toBe('closed');
  });
});
