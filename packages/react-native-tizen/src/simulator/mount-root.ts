import { sep } from 'node:path';
import { FixableError } from '../errors.ts';

/**
 * The deepest directory holding every one of `dirs`, which must be absolute:
 * the one mount that lets the VM see them all at their own paths.
 */
export function mountRoot(dirs: readonly string[]): string {
  const [first, ...rest] = dirs.map((dir) => dir.split(sep));
  if (first === undefined) throw new Error('mountRoot() needs a directory');
  let length = first.length;
  for (const parts of rest) {
    let shared = 0;
    while (shared < Math.min(length, parts.length) && parts[shared] === first[shared]) shared += 1;
    length = shared;
  }
  return first.slice(0, length).join(sep) || sep;
}

/**
 * Fails unless `root` is a directory below `home`: the VM would otherwise see
 * every file of the user, or of the machine.
 */
export function requireNarrowMount(root: string, home: string): void {
  const homeParts = home.split(sep).filter(Boolean);
  const rootParts = root.split(sep).filter(Boolean);
  const coversHome =
    rootParts.length <= homeParts.length &&
    rootParts.every((part, index) => part === homeParts[index]);
  if (!coversHome) return;
  throw new FixableError(
    `The simulator would mount ${root}, which holds ${home}`,
    'Keep the app, its node_modules and react-native-tizen in one project directory inside your home directory.',
  );
}

/** True when `dir` is `mount` or inside it. */
export function isWithin(dir: string, mount: string): boolean {
  if (dir === mount || mount === sep) return true;
  return dir.startsWith(mount.endsWith(sep) ? mount : `${mount}${sep}`);
}
