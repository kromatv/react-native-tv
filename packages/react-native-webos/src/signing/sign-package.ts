import { readFile, writeFile } from 'node:fs/promises';
import {
  AUTHOR_SIGNATURE,
  distributorSignatureName,
  isSignatureFile,
  signedFiles,
} from './package-files.ts';
import { signatureXml, type SigningKey } from './signature-xml.ts';
import { readZip, type ZipEntry } from './zip-read.ts';
import { writeZip } from './zip-write.ts';

/** The keys a package is signed with: one author, and one or two distributors. */
export interface SigningIdentity {
  readonly author: SigningKey;
  readonly distributors: readonly SigningKey[];
}

/**
 * Signs a package's files in place: any earlier signature removed, then the
 * author signature, then each distributor's, which covers the author's.
 */
export function signFiles(files: Map<string, Buffer>, identity: SigningIdentity): void {
  for (const path of files.keys()) if (isSignatureFile(path)) files.delete(path);
  const author = signatureXml(signedFiles(files, 'author'), 'author', identity.author);
  files.set(AUTHOR_SIGNATURE, Buffer.from(author, 'utf8'));
  const distributorFiles = signedFiles(files, 'distributor');
  identity.distributors.forEach((distributor, index) => {
    const xml = signatureXml(distributorFiles, 'distributor', distributor);
    files.set(distributorSignatureName(index), Buffer.from(xml, 'utf8'));
  });
}

function directoriesOf(paths: Iterable<string>): Set<string> {
  const directories = new Set<string>();
  for (const path of paths) {
    const parts = path.split('/');
    for (let depth = 1; depth < parts.length; depth++)
      directories.add(parts.slice(0, depth).join('/'));
  }
  return directories;
}

/** The zip a signed package is written as: every directory and file, sorted by name. */
export function packageEntries(files: ReadonlyMap<string, Buffer>): ZipEntry[] {
  const directories: ZipEntry[] = [...directoriesOf(files.keys())].map((name) => ({
    name,
    directory: true,
    data: Buffer.alloc(0),
  }));
  const plain: ZipEntry[] = [...files].map(([name, data]) => ({ name, directory: false, data }));
  const sortName = (entry: ZipEntry) => (entry.directory ? `${entry.name}/` : entry.name);
  return [...directories, ...plain].sort((left, right) =>
    sortName(left) < sortName(right) ? -1 : 1,
  );
}

/** Re-signs a `.tpk` or `.wgt` with `identity`, writing it to `out`. */
export async function signPackage(
  path: string,
  identity: SigningIdentity,
  out = path,
): Promise<void> {
  const files = new Map<string, Buffer>();
  for (const entry of readZip(await readFile(path)))
    if (!entry.directory) files.set(entry.name, entry.data);
  signFiles(files, identity);
  await writeFile(out, writeZip(packageEntries(files)));
}
