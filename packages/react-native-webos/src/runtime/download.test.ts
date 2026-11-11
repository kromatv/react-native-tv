import { createHash } from 'node:crypto';
import { existsSync, mkdtempSync, readdirSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { FixableError } from '../errors.ts';
import { downloadVerified } from './download.ts';

const URL = 'https://example.org/archive.tar.gz';
const BODY = 'archive bytes';
const DIGEST = createHash('sha256').update(BODY).digest('hex');

let dir: string;
let destination: string;

function serve(status: number, body: string | null) {
  vi.stubGlobal(
    'fetch',
    vi.fn(async () => new Response(body, { status })),
  );
}

beforeEach(() => {
  dir = mkdtempSync(join(tmpdir(), 'rn-tizen-download-'));
  destination = join(dir, 'downloads', 'archive.tar.gz');
});

afterEach(() => {
  rmSync(dir, { recursive: true, force: true });
  vi.unstubAllGlobals();
});

describe('downloadVerified', () => {
  it('saves the archive once its digest matches', async () => {
    serve(200, BODY);

    await downloadVerified(URL, DIGEST, destination);

    expect(readFileSync(destination, 'utf8')).toBe(BODY);
  });

  it('fetches nothing when the file there already has the digest', async () => {
    serve(200, BODY);
    await downloadVerified(URL, DIGEST, destination);

    await downloadVerified(URL, DIGEST, destination);

    expect(fetch).toHaveBeenCalledTimes(1);
  });

  it('keeps nothing of an archive whose digest does not match', async () => {
    serve(200, 'tampered');

    await expect(downloadVerified(URL, DIGEST, destination)).rejects.toThrow(
      'does not match its pinned digest',
    );
    expect(readdirSync(join(dir, 'downloads'))).toEqual([]);
  });

  it('says to check the network on an HTTP error', async () => {
    serve(404, 'missing');

    await expect(downloadVerified(URL, DIGEST, destination)).rejects.toThrow('HTTP 404');
    expect(existsSync(destination)).toBe(false);
  });

  it('says to check the network when the server cannot be reached', async () => {
    vi.stubGlobal(
      'fetch',
      vi.fn(async () => {
        throw new TypeError('fetch failed');
      }),
    );

    const downloading = downloadVerified(URL, DIGEST, destination);

    await expect(downloading).rejects.toBeInstanceOf(FixableError);
    await expect(downloading).rejects.toThrow(`Downloading ${URL} failed: fetch failed`);
  });

  it('replaces a file there whose digest is wrong', async () => {
    serve(200, BODY);
    await downloadVerified(URL, DIGEST, destination);
    writeFileSync(destination, 'stale');

    await downloadVerified(URL, DIGEST, destination);

    expect(readFileSync(destination, 'utf8')).toBe(BODY);
  });
});
