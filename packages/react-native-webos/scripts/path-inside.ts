import { resolve, sep } from 'node:path';
import { FixableError } from '../src/errors.ts';

/**
 * `path` made absolute, when it lies inside one of `roots`. Throws for a path
 * anywhere else, so a command-line argument cannot make a script write
 * outside the directories it owns.
 */
export function pathInside(path: string, roots: readonly string[]): string {
  const resolved = resolve(path);
  const bases = roots.map((root) => resolve(root));
  for (const base of bases) {
    if (resolved.startsWith(base + sep)) return resolved;
  }
  throw new FixableError(
    `${resolved} is outside ${bases.join(' and ')}`,
    'Pass a path inside the repository, such as coverage/runtime.xml',
  );
}
