import { mkdtempSync, symlinkSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { fileURLToPath } from 'node:url';

const EXAMPLE_MODULES = fileURLToPath(new URL('../../example/node_modules', import.meta.url));

/** A temporary app with the example's packages, whose Expo config is `expo`. */
export function appDir(expo: Record<string, unknown>): string {
  const dir = mkdtempSync(join(tmpdir(), 'rn-tizen-app-'));
  symlinkSync(EXAMPLE_MODULES, join(dir, 'node_modules'), 'dir');
  writeFileSync(
    join(dir, 'package.json'),
    JSON.stringify({ name: 'fixture-app', version: '1.0.0', main: 'index.js' }),
  );
  writeFileSync(join(dir, 'index.js'), '');
  writeFileSync(join(dir, 'app.json'), JSON.stringify({ expo }));
  return dir;
}
