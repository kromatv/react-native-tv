import { fileURLToPath } from 'node:url';
import { writePlatformApi } from './write-platform-api.ts';

const packageDir = fileURLToPath(new URL('../../', import.meta.url));
const written = writePlatformApi(`${packageDir}package.json`, `${packageDir}dist/api`);
console.log(`Platform API modules: ${written.join(', ') || 'none'}`);
