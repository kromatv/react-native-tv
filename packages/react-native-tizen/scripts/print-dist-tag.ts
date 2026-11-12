import { readFileSync } from 'node:fs';
import { z } from 'zod';
import { distTag } from './dist-tag.ts';

const MAX_PACKUMENT_BYTES = 32 * 1024 * 1024;

const Manifest = z.object({ name: z.string(), version: z.string() });
const Packument = z.object({ versions: z.record(z.string(), z.unknown()) });

async function publishedVersions(name: string): Promise<string[]> {
  const url = `https://registry.npmjs.org/${name.replace('/', '%2f')}`;
  const response = await fetch(url, {
    headers: { accept: 'application/vnd.npm.install-v1+json' },
  });
  if (response.status === 404) return [];
  if (!response.ok) throw new Error(`${url} answered ${response.status}`);
  const text = await response.text();
  if (text.length > MAX_PACKUMENT_BYTES) throw new Error(`${url} sent more than 32 MB`);
  return Object.keys(Packument.parse(JSON.parse(text)).versions);
}

const { name, version } = Manifest.parse(JSON.parse(readFileSync('package.json', 'utf8')));
console.log(`dist-tag=${distTag(version, await publishedVersions(name))}`);
