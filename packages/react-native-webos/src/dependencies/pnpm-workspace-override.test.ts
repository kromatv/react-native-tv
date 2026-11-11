import { existsSync, mkdtempSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, describe, expect, it } from 'vitest';
import { setPnpmWorkspaceOverride } from './pnpm-workspace-override.ts';

const dirs: string[] = [];

afterEach(() => {
  for (const dir of dirs.splice(0)) rmSync(dir, { recursive: true, force: true });
});

function workspace(text?: string): string {
  const dir = mkdtempSync(join(tmpdir(), 'rn-tizen-pnpm-'));
  dirs.push(dir);
  if (text !== undefined) writeFileSync(join(dir, 'pnpm-workspace.yaml'), text);
  return dir;
}

const yaml = (dir: string) => readFileSync(join(dir, 'pnpm-workspace.yaml'), 'utf8');

describe('setPnpmWorkspaceOverride', () => {
  it('writes the file when there is none, then finds the override there', () => {
    const dir = workspace();

    expect(setPnpmWorkspaceOverride(dir, '$react-native').kind).toBe('set');
    expect(yaml(dir)).toBe("overrides:\n  react-native: '$react-native'\n");
    expect(setPnpmWorkspaceOverride(dir, '$react-native').kind).toBe('present');
  });

  it('adds overrides after the settings a workspace has', () => {
    const dir = workspace("packages:\n  - 'apps/*'");

    setPnpmWorkspaceOverride(dir, 'npm:react-native-tvos@0.88.0-0');

    expect(yaml(dir)).toBe(
      "packages:\n  - 'apps/*'\noverrides:\n  react-native: 'npm:react-native-tvos@0.88.0-0'\n",
    );
  });

  it('adds to an overrides block at its indentation, and updates an entry that differs', () => {
    const dir = workspace('overrides: # pinned\n    # a comment\n    foo: 1.0.0\ncatalog: {}\n');

    setPnpmWorkspaceOverride(dir, '$react-native');
    const added = yaml(dir);
    setPnpmWorkspaceOverride(dir, 'npm:react-native-tvos@0.88.0-0');

    expect(added).toBe(
      "overrides: # pinned\n    react-native: '$react-native'\n    # a comment\n    foo: 1.0.0\ncatalog: {}\n",
    );
    expect(yaml(dir)).toContain(
      "    react-native: 'npm:react-native-tvos@0.88.0-0'\n    # a comment",
    );
  });

  it('reads an entry quoted either way or not at all, with a comment after it', () => {
    for (const entry of ['"react-native": "$react-native"', 'react-native: $react-native # tv']) {
      const dir = workspace(`overrides:\n  ${entry}\n`);

      expect(setPnpmWorkspaceOverride(dir, '$react-native').kind).toBe('present');
    }
  });

  it('gives the lines to add when overrides is written inline', () => {
    const dir = workspace('overrides: { foo: 1.0.0 }\n');

    expect(setPnpmWorkspaceOverride(dir, '$react-native')).toEqual({
      kind: 'manual',
      file: join(dir, 'pnpm-workspace.yaml'),
      lines: ['overrides:', "  react-native: '$react-native'"],
    });
    expect(yaml(dir)).toBe('overrides: { foo: 1.0.0 }\n');
  });

  it('refuses a file too large to be a workspace', () => {
    const dir = workspace('#'.repeat(1024 * 1024 + 1));

    expect(() => setPnpmWorkspaceOverride(dir, '$react-native')).toThrow('is too large');
    expect(existsSync(join(dir, 'pnpm-workspace.yaml'))).toBe(true);
  });
});
