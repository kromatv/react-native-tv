import { join, resolve } from 'node:path';
import { PACKAGE_ROOT } from '../package-paths.ts';
import { TIZEN_PLUGIN } from './tizen-settings.ts';

const PLUGIN_FILE = join(PACKAGE_ROOT, 'app.plugin.js');

/** This package's plugin by name, or by a path to its `app.plugin.js`, as an app beside it lists it. */
export function isTizenPluginEntry(root: string, name: string): boolean {
  return name === TIZEN_PLUGIN || (name.startsWith('.') && resolve(root, name) === PLUGIN_FILE);
}
