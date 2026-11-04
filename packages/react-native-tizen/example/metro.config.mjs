import { resolve } from 'node:path';
import { getDefaultConfig } from 'expo/metro-config';
import { withTizen } from '../src/metro/with-tizen.ts';

const workspaceRoot = resolve(import.meta.dirname, '..');
const config = getDefaultConfig(import.meta.dirname);
config.watchFolders = [workspaceRoot];
config.resolver.nodeModulesPaths = [
  resolve(import.meta.dirname, 'node_modules'),
  resolve(workspaceRoot, 'node_modules'),
];
config.resolver.unstable_enableSymlinks = true;

export default withTizen(config);
