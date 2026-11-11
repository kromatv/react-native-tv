import { join, resolve } from 'node:path';
import { PACKAGE_ROOT } from '../package-paths.ts';
import { WEBOS_PLUGIN } from './webos-settings.ts';

const PLUGIN_FILE = join(PACKAGE_ROOT, 'app.plugin.js');

/** This package's plugin by name, or by a path to its `app.plugin.js`, as an app beside it lists it. */
export function isWebOSPluginEntry(root: string, name: string): boolean {
  return name === WEBOS_PLUGIN || (name.startsWith('.') && resolve(root, name) === PLUGIN_FILE);
}
