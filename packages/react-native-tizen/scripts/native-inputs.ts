import { existsSync, readFileSync } from 'node:fs';
import { dirname, join, normalize } from 'node:path';

const ENTRIES = ['scripts/compile-simulator-runtime.ts', 'scripts/test-runtime.ts'];
const NATIVE_ROOTS = [
  'runtime/',
  'simulator/',
  'react-native-overrides/',
  'example/package.json',
  'example/app.json',
  'example/app.config.',
  'example/tizen/',
  'package.json',
  'bun.lock',
  '.bun-version',
  '.github/workflows/ci.yml',
];
const RELATIVE_IMPORT = /(?:from |import\(|import )['"](\.\.?\/[^'"]+)['"]/g;

function importClosure(entries: readonly string[]): Set<string> {
  const seen = new Set<string>();
  const pending = [...entries];
  while (pending.length > 0) {
    const file = pending.pop();
    if (file === undefined || seen.has(file) || !existsSync(file)) continue;
    seen.add(file);
    for (const match of readFileSync(file, 'utf8').matchAll(RELATIVE_IMPORT)) {
      const specifier = match[1];
      if (specifier !== undefined) pending.push(normalize(join(dirname(file), specifier)));
    }
  }
  return seen;
}

const changed = readFileSync(0, 'utf8')
  .split('\n')
  .map((line) => line.trim())
  .filter((line) => line !== '');
const inputs = importClosure(ENTRIES);
const touched = changed.filter(
  (file) => inputs.has(file) || NATIVE_ROOTS.some((root) => file.startsWith(root)),
);
for (const file of touched) console.error(`native input changed: ${file}`);
console.log(`native=${touched.length > 0}`);
