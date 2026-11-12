import { mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, describe, expect, it } from 'vitest';
import { fakeTv, type FakeTv, type FakeTvOptions } from './fake-tv.fixtures.ts';
import { pushFile } from './push-file.ts';
import { openSshSession } from './ssh-session.ts';

const tvs: FakeTv[] = [];
const dirs: string[] = [];
const REMOTE = '/media/developer/temp/rn-webos-0123456789.ipk';

function localFile(bytes: number): string {
  const dir = mkdtempSync(join(tmpdir(), 'rn-webos-push-'));
  dirs.push(dir);
  const path = join(dir, 'app.ipk');
  writeFileSync(path, Buffer.alloc(bytes, 7));
  return path;
}

async function push(options: FakeTvOptions, bytes: number, remote = REMOTE) {
  const tv = await fakeTv(options);
  tvs.push(tv);
  const session = await openSshSession({
    host: '127.0.0.1',
    port: tv.port,
    privateKey: tv.privateKey,
  });
  const progress: number[] = [];
  try {
    await pushFile(session, localFile(bytes), remote, (sent, total) => {
      progress.push(Math.floor((sent * 100) / total));
    });
  } finally {
    session.close();
  }
  return { tv, progress };
}

afterEach(async () => {
  await Promise.all(tvs.splice(0).map((tv) => tv.close()));
  for (const dir of dirs.splice(0)) rmSync(dir, { recursive: true, force: true });
});

describe('pushFile', () => {
  it('sends the file through SFTP when the TV has it', async () => {
    const { tv, progress } = await push({}, 300_000);

    expect(tv.transfers).toEqual(['sftp']);
    expect(tv.files.get(REMOTE)?.equals(Buffer.alloc(300_000, 7))).toBe(true);
    expect(progress.at(-1)).toBe(100);
  });

  it('falls back to cat on a TV without sftp-server', async () => {
    const { tv, progress } = await push({ sftp: false }, 300_000);

    expect(tv.transfers).toEqual(['cat']);
    expect(tv.commands).toEqual([`/bin/cat > "${REMOTE}"`]);
    expect(tv.files.get(REMOTE)?.equals(Buffer.alloc(300_000, 7))).toBe(true);
    expect(progress.at(-1)).toBe(100);
  });

  it('refuses a remote path that would need quoting', async () => {
    await expect(push({}, 1, '/media/developer/temp/a b.ipk')).rejects.toThrow('refusing to write');
  });
});
