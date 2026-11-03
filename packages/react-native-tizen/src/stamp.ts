import { readFile, rm, writeFile } from 'node:fs/promises';

interface Stamp {
  matches(fingerprint: string): Promise<boolean>;
  record(fingerprint: string): Promise<void>;
  clear(): Promise<void>;
}

/**
 * A file at `path` recording what an expensive step was last run with, so the
 * step can be skipped when nothing changed. A missing stamp never matches.
 */
export function stampAt(path: string): Stamp {
  return {
    async matches(fingerprint) {
      try {
        return (await readFile(path, 'utf8')) === fingerprint;
      } catch {
        return false;
      }
    },
    record: (fingerprint) => writeFile(path, fingerprint),
    clear: () => rm(path, { force: true }),
  };
}
