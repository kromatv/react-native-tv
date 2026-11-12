import { join } from 'node:path';
import { cacheDir } from '../cache-dir.ts';
import type { Task } from '../task.ts';
import { archiveFileName, pinnedDependency } from './dependencies.ts';
import { downloadVerified } from './download.ts';

const PIN_NAME = 'cacert';

/**
 * Mozilla's root certificates as curl publishes them, the pinned release,
 * downloaded once and checked against its digest. Every package ships it: the
 * runtime's own TLS trusts this file, not the TV's root list, which is as old
 * as its firmware. Returns its path.
 */
export async function ensureCaBundle(task: Task): Promise<string> {
  const pin = pinnedDependency(PIN_NAME);
  const file = join(cacheDir(), 'downloads', PIN_NAME, archiveFileName(PIN_NAME, pin));
  task.progress('downloading');
  await downloadVerified(pin.url, pin.sha256, file);
  return file;
}
