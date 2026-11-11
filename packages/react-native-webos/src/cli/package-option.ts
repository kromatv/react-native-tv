import { existsSync } from 'node:fs';
import { join } from 'node:path';
import { loadAppProject } from '../app/app-project.ts';
import { packageIdIssue } from '../config/package-id.ts';
import { UsageError, type OptionSpecs } from './command.ts';

/** The option a command that reads one app on the TV takes. */
export const PACKAGE_OPTIONS = {
  package: {
    type: 'string',
    placeholder: '<id>',
    description:
      "The app to read, by its Tizen package id (default: the app in --project or the current directory's, else the only one connected)",
  },
} as const satisfies OptionSpecs;

const CONFIG_FILES = ['app.json', 'app.config.js', 'app.config.ts'];

/**
 * The package a command reads: `--package`, else the app's in `dir`, else
 * null, which takes the only app connected.
 */
export function targetPackage(
  flag: string | undefined,
  dir: string,
  command: string,
): string | null {
  if (flag !== undefined) {
    const issue = packageIdIssue(flag);
    if (issue !== null)
      throw new UsageError(`--package is not a Tizen package id: ${issue}`, command);
    return flag;
  }
  if (!CONFIG_FILES.some((name) => existsSync(join(dir, name)))) return null;
  try {
    return loadAppProject(dir).config.packageId;
  } catch {
    return null;
  }
}
