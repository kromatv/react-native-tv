import { createRequire } from 'node:module';

const require = createRequire(import.meta.url);

// A checkout of this repository has no dist/; rn-tizen still checks the props when it reads them.
function loadPlugin() {
  try {
    return require('./dist/config/config-plugin.js').default;
  } catch (error) {
    if (error?.code !== 'MODULE_NOT_FOUND') throw error;
    return (config) => config;
  }
}

export default loadPlugin();
