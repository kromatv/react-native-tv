const path = require('node:path');
const { getDefaultConfig } = require('expo/metro-config');
const { withTizen } = require('../src/metro/with-tizen.ts');

const workspaceRoot = path.resolve(__dirname, '..');
const config = getDefaultConfig(__dirname);
config.watchFolders = [workspaceRoot];
config.resolver.nodeModulesPaths = [
  path.resolve(__dirname, 'node_modules'),
  path.resolve(workspaceRoot, 'node_modules'),
];
config.resolver.unstable_enableSymlinks = true;

module.exports = withTizen(config);
