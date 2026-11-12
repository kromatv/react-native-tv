import { TV_PROJECT_ROOT } from './expo-metro-override.ts';
import { loadAppMetroConfig } from './load-app-metro-config.ts';

export default loadAppMetroConfig(process.env[TV_PROJECT_ROOT] ?? process.cwd());
