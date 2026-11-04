import { createHash } from 'node:crypto';
import { readFile } from 'node:fs/promises';
import { readZip } from '../signing/zip-read.ts';
import { RUNTIME_LIBRARY } from '../runtime/build-runtime.ts';

const PACKAGED_PATH = `lib/${RUNTIME_LIBRARY}`;

const sha256 = (bytes: Buffer) => createHash('sha256').update(bytes).digest('hex');

/**
 * Throws unless the `.tpk` carries exactly the runtime library that was built
 * for it, so a stale copy of the other flavour never ships under its name.
 */
export async function checkPackagedRuntime(
  tpk: string,
  runtimeLibrary: string,
  flavour: string,
): Promise<void> {
  const entry = readZip(await readFile(tpk)).find((candidate) => candidate.name === PACKAGED_PATH);
  if (entry === undefined) throw new Error(`${tpk} holds no ${PACKAGED_PATH}`);
  const built = sha256(await readFile(runtimeLibrary));
  if (sha256(entry.data) === built) return;
  throw new Error(
    `${tpk} carries another ${RUNTIME_LIBRARY} than the ${flavour} one built for it (${built.slice(0, 12)})`,
  );
}
