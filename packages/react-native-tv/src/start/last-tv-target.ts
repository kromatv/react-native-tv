import { mkdirSync, readFileSync, writeFileSync } from 'node:fs';
import { homedir } from 'node:os';
import { dirname, join } from 'node:path';
import { z } from 'zod';

/** The target `t` opened last in a project: a platform, and what that platform calls the target. */
export interface LastTvTarget {
  readonly platform: string;
  readonly target: string;
}

const MAX_FILE_BYTES = 1024 * 1024;
const MAX_PROJECTS = 100;
const storeSchema = z.record(
  z.string(),
  z.object({ platform: z.string().max(32), target: z.string().max(256) }),
);

/** The file that remembers the last target of each project; `RN_TV_CACHE_DIR` overrides its directory. */
export function lastTargetFile(env: NodeJS.ProcessEnv = process.env): string {
  let dir = join(homedir(), '.cache', 'react-native-tv');
  if (env.RN_TV_CACHE_DIR) dir = env.RN_TV_CACHE_DIR;
  else if (env.XDG_CACHE_HOME) dir = join(env.XDG_CACHE_HOME, 'react-native-tv');
  return join(dir, 'last-target.json');
}

function readStore(file: string): Record<string, LastTvTarget> {
  try {
    const text = readFileSync(file, 'utf8');
    if (text.length > MAX_FILE_BYTES) return {};
    const store = storeSchema.safeParse(JSON.parse(text));
    return store.success ? store.data : {};
  } catch {
    return {};
  }
}

/** The target opened last in the project at `projectRoot`, or null. */
export function lastTvTarget(file: string, projectRoot: string): LastTvTarget | null {
  return readStore(file)[projectRoot] ?? null;
}

/** Remembers `target` for the project, keeping the hundred projects written last. A failure to write is ignored. */
export function rememberTvTarget(file: string, projectRoot: string, target: LastTvTarget): void {
  const others = Object.entries(readStore(file)).filter(([root]) => root !== projectRoot);
  const kept = Object.fromEntries([...others.slice(1 - MAX_PROJECTS), [projectRoot, target]]);
  try {
    mkdirSync(dirname(file), { recursive: true });
    writeFileSync(file, JSON.stringify(kept));
  } catch {
    return;
  }
}
