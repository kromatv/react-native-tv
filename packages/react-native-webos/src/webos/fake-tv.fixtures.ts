import { createHash } from 'node:crypto';
import ssh2, { type Algorithms, type Connection, type ServerChannel, type Session } from 'ssh2';
import { rsaKeyPem } from './developer-key.fixtures.ts';
import { acceptSftp } from './fake-tv-sftp.fixtures.ts';

/** Answers one luna call with the replies `luna-send-pub` prints, one JSON object a line. */
export type LunaHandler = (params: Record<string, unknown>, tv: FakeTv) => readonly object[];

export interface FakeTvOptions {
  /** `false` is a TV without `sftp-server`: the subsystem request is refused. */
  readonly sftp?: boolean;
  /** `missing` is a TV without `md5sum`; `wrong` one that received other bytes than were sent. */
  readonly md5sum?: 'missing' | 'wrong';
  /** `false` refuses every key, as a TV whose Developer Mode app was reinstalled. */
  readonly acceptKey?: boolean;
  /** Restricts what the server negotiates, to stand for an old sshd. */
  readonly algorithms?: Algorithms;
  /** Replaces the answer to a call, by `service/category/method`. */
  readonly luna?: Readonly<Record<string, LunaHandler>>;
}

/** A local SSH server that answers as a TV in Developer Mode, with the key it accepts. */
export interface FakeTv {
  readonly port: number;
  readonly privateKey: string;
  /** Every command line the host ran, in order. */
  readonly commands: string[];
  /** The files the host wrote and did not remove. */
  readonly files: Map<string, Buffer>;
  /** How each file arrived: through the SFTP subsystem or `cat`. */
  readonly transfers: ('sftp' | 'cat')[];
  /** The bytes at `ipkUrl` each time an install was asked for. */
  readonly installed: Buffer[];
  connections: number;
  close(): Promise<void>;
}

/** The status lines an OLED55CX5LB on webOS 5.6.2 printed for one install (webos-device-protocol.md, 6.3). */
export const INSTALL_STATES: readonly (readonly [number, string])[] = [
  [268, 'ipk verifying'],
  [35, 'ipk parsing'],
  [37, 'ipk parsing'],
  [32, 'app closing'],
  [11, 'installing'],
  [13, 'installing : start'],
  [17, 'installing : installing'],
  [27, 'installing : service installing'],
  [30, 'installed'],
];

const SUBSCRIBED = { subscribed: true, returnValue: true };

function installStatus(statusValue: number, state: string, more: object = {}): object {
  return {
    id: 'com.ares.defaultName',
    statusValue,
    details: { state, installBasePath: '/media/developer', verified: false, ...more },
  };
}

/** A failed install as the open installer reports it: state `install failed`, with a reason. */
export function failedInstall(reason: string, errorCode = -5): LunaHandler {
  return () => [SUBSCRIBED, installStatus(24, 'install failed', { errorCode, reason })];
}

const DEFAULT_LUNA: Readonly<Record<string, LunaHandler>> = {
  'com.webos.appInstallService/dev/install': (params, tv) => {
    tv.installed.push(tv.files.get(String(params.ipkUrl)) ?? Buffer.alloc(0));
    return [
      SUBSCRIBED,
      ...INSTALL_STATES.map(([value, state], index) =>
        installStatus(value, state, index < 2 ? {} : { packageId: 'com.example.myapp' }),
      ),
    ];
  },
  'com.webos.appInstallService/dev/remove': (params) => [
    SUBSCRIBED,
    { id: params.id, statusValue: 31, details: { state: 'removed' } },
  ],
  'com.webos.applicationManager/launch': (params) => [
    { returnValue: true, appId: params.id, instanceId: 'a1b2c3d40', displayId: 0 },
  ],
  'com.webos.applicationManager/dev/closeByAppId': () => [{ returnValue: true }],
  'com.webos.applicationManager/dev/listApps': () => [
    {
      returnValue: true,
      apps: [
        {
          id: 'com.example.myapp',
          version: '1.0.0',
          type: 'native',
          title: 'My app',
          visible: true,
          folderPath: '/media/developer/apps/usr/palm/applications/com.example.myapp',
        },
        {
          id: 'com.example.hidden',
          version: '0.1.0',
          type: 'web',
          title: 'Hidden',
          visible: false,
        },
      ],
    },
  ],
  'com.webos.applicationManager/dev/running': () => [
    { returnValue: true, running: [{ id: 'com.example.myapp', processid: '4242' }] },
  ],
  'com.webos.service.tv.systemproperty/getSystemInfo': () => [
    {
      returnValue: true,
      modelName: 'OLED65C25LB',
      sdkVersion: '11.2.0',
      firmwareVersion: '43.21.74',
      boardType: 'O22_DVB',
      otaId: 'HE_DTV_W22O_AFABATAA',
    },
  ],
};

interface Keys {
  readonly host: string;
  readonly client: string;
}

let keys: Keys | null = null;

function testKeys(): Keys {
  keys ??= { host: rsaKeyPem(), client: rsaKeyPem() };
  return keys;
}

const LUNA = /^\/usr\/bin\/luna-send-pub (-n 1|-i) luna:\/\/(\S+) '(.*)'$/s;
const CAT = /^\/bin\/cat > "([^"]+)"$/;
const MD5 = /^\/usr\/bin\/md5sum (\S+)$/;
const RM = /^\/bin\/rm -f (\S+)$/;

function finish(channel: ServerChannel, code: number, stderr = ''): void {
  if (stderr !== '') channel.stderr.write(stderr);
  channel.exit(code);
  channel.end();
}

function runLuna(tv: FakeTv, channel: ServerChannel, match: RegExpExecArray, luna: LunaLookup) {
  const [, mode = '', uri = '', json = ''] = match;
  const handler = luna(uri.replace(/\/{2,}/g, '/'));
  const params: Record<string, unknown> = JSON.parse(json.replaceAll(`'\\''`, `'`));
  const replies = handler?.(params, tv) ?? [
    { returnValue: false, errorCode: -1, errorText: `Unknown method "${uri}"` },
  ];
  if (mode === '-n 1') {
    channel.write(`${JSON.stringify(replies[0])}\n`);
    finish(channel, 0);
    return;
  }
  for (const reply of replies) channel.write(`${JSON.stringify(reply)}\n`);
  channel.on('end', () => finish(channel, 0)).resume();
}

type LunaLookup = (uri: string) => LunaHandler | undefined;

function runCommand(
  tv: FakeTv,
  channel: ServerChannel,
  command: string,
  luna: LunaLookup,
  md5sum: FakeTvOptions['md5sum'],
): void {
  const lunaCall = LUNA.exec(command);
  const cat = CAT.exec(command)?.[1];
  const hashed = md5sum === 'missing' ? undefined : MD5.exec(command)?.[1];
  const removed = RM.exec(command)?.[1];
  if (lunaCall !== null) {
    runLuna(tv, channel, lunaCall, luna);
  } else if (cat !== undefined) {
    const chunks: Buffer[] = [];
    channel.on('data', (chunk: Buffer) => chunks.push(chunk));
    channel.on('end', () => {
      tv.files.set(cat, Buffer.concat(chunks));
      tv.transfers.push('cat');
      finish(channel, 0);
    });
  } else if (hashed !== undefined) {
    const file = tv.files.get(hashed);
    if (file === undefined) finish(channel, 1, `md5sum: ${hashed}: No such file or directory\n`);
    else {
      // oxlint-disable-next-line sonarjs/hashing
      const digest = createHash('md5') // NOSONAR: what md5sum prints
        .update(md5sum === 'wrong' ? 'other bytes' : file)
        .digest('hex');
      channel.write(`${digest}  ${hashed}\n`);
      finish(channel, 0);
    }
  } else if (removed !== undefined) {
    tv.files.delete(removed);
    finish(channel, 0);
  } else if (command.startsWith('/usr/bin/test -d ')) {
    finish(channel, 0);
  } else {
    finish(channel, 127, `sh: ${command.split(' ')[0]}: not found\n`);
  }
}

function serve(tv: FakeTv, connection: Connection, options: FakeTvOptions, keyPem: string): void {
  const allowed = ssh2.utils.parseKey(keyPem);
  if (allowed instanceof Error) throw allowed;
  const luna: LunaLookup = (uri) => ({ ...DEFAULT_LUNA, ...options.luna })[uri];
  connection.on('error', () => undefined);
  connection.on('authentication', (context) => {
    const offered = context.method === 'publickey' ? context.key.data : null;
    const known = offered?.equals(allowed.getPublicSSH()) === true;
    if (context.username !== 'prisoner' || !known || options.acceptKey === false) {
      context.reject(['publickey']);
      return;
    }
    context.accept();
  });
  connection.on('ready', () => {
    tv.connections += 1;
    connection.on('session', (accept) => {
      const session: Session = accept();
      session.on('exec', (acceptExec, _reject, info) => {
        tv.commands.push(info.command);
        runCommand(tv, acceptExec(), info.command, luna, options.md5sum);
      });
      session.on('sftp', (acceptSftpSession, reject) => {
        if (options.sftp === false) reject();
        else acceptSftp(acceptSftpSession(), tv);
      });
    });
  });
}

/** Starts a fake TV on a free local port. */
export async function fakeTv(options: FakeTvOptions = {}): Promise<FakeTv> {
  const { host, client } = testKeys();
  const connections = new Set<Connection>();
  const server = new ssh2.Server({ hostKeys: [host], algorithms: options.algorithms });
  await new Promise<void>((resolve) => server.listen(0, '127.0.0.1', resolve));
  const address = server.address();
  if (address === null || typeof address === 'string') throw new Error('the fake TV got no port');
  const tv: FakeTv = {
    port: address.port,
    privateKey: client,
    commands: [],
    files: new Map(),
    transfers: [],
    installed: [],
    connections: 0,
    close: () =>
      new Promise((resolve) => {
        for (const connection of connections) connection.end();
        server.close(() => resolve());
      }),
  };
  server.on('connection', (connection: Connection) => {
    connections.add(connection);
    connection.on('close', () => connections.delete(connection));
    serve(tv, connection, options, client);
  });
  return tv;
}
