import { readFileSync, writeFileSync } from 'node:fs';
import { join } from 'node:path';
import { CANARY_PACKAGES, canaryVersion, stampCanary } from './canary.ts';

const [build, sha] = process.argv.slice(2);
const version = canaryVersion(Number(build), sha ?? '');
const packages = join(import.meta.dirname, '..', '..');

for (const name of CANARY_PACKAGES) {
  const file = join(packages, name, 'package.json');
  writeFileSync(file, stampCanary(readFileSync(file, 'utf8'), version));
}
console.log(`version=${version}`);
