import { existsSync, readFileSync, writeFileSync } from 'node:fs';
import { join } from 'node:path';
import { FixableError } from '../errors.ts';
import type { OverrideSetup } from './override-setup.ts';

const MAX_WORKSPACE_BYTES = 1024 * 1024;
const BLOCK_START = /^overrides\s*:\s*(?:#.*)?$/;
const INLINE_START = /^overrides\s*:/;
const KEYS = ['react-native', '"react-native"', "'react-native'"];

function entryLine(indent: string, value: string): string {
  return `${indent}react-native: '${value}'`;
}

function entryOf(line: string): { readonly indent: string; readonly value: string } | null {
  const indent = /^\s+/.exec(line)?.[0];
  if (indent === undefined) return null;
  const rest = line.slice(indent.length);
  const key = KEYS.find((candidate) => rest.startsWith(candidate));
  const after = key === undefined ? '' : rest.slice(key.length).trimStart();
  return after.startsWith(':') ? { indent, value: after.slice(1) } : null;
}

function scalar(text: string): string {
  const comment = text.search(/\s#/);
  const value = (comment === -1 ? text : text.slice(0, comment)).trim();
  const quoted =
    value.length > 1 && ['"', "'"].some((q) => value.startsWith(q) && value.endsWith(q));
  return quoted ? value.slice(1, -1) : value;
}

function isBlockLine(line: string): boolean {
  return line.trim() === '' || /^\s/.test(line);
}

/**
 * Sets `overrides.react-native` in `pnpm-workspace.yaml`, where pnpm 11 and
 * later read overrides, creating the file when there is none. An `overrides`
 * written inline is the user's to change.
 */
export function setPnpmWorkspaceOverride(root: string, value: string): OverrideSetup {
  const file = join(root, 'pnpm-workspace.yaml');
  if (!existsSync(file)) {
    writeFileSync(file, `overrides:\n${entryLine('  ', value)}\n`);
    return { kind: 'set', file };
  }
  const text = readFileSync(file, 'utf8');
  if (text.length > MAX_WORKSPACE_BYTES)
    throw new FixableError(`${file} is too large`, `Fix ${file}.`);
  const lines = text.split('\n');
  const start = lines.findIndex((line) => INLINE_START.test(line));
  if (start === -1) {
    const separator = text === '' || text.endsWith('\n') ? '' : '\n';
    writeFileSync(file, `${text}${separator}overrides:\n${entryLine('  ', value)}\n`);
    return { kind: 'set', file };
  }
  if (!BLOCK_START.test(lines[start] ?? '')) {
    return { kind: 'manual', file, lines: ['overrides:', entryLine('  ', value)] };
  }
  const after = lines.slice(start + 1);
  const length = after.findIndex((line) => !isBlockLine(line));
  const block = length === -1 ? after : after.slice(0, length);
  const existing = block.findIndex((line) => entryOf(line) !== null);
  const entry = entryOf(block[existing] ?? '');
  if (entry !== null) {
    if (scalar(entry.value) === value) return { kind: 'present', file };
    lines[start + 1 + existing] = entryLine(entry.indent, value);
  } else {
    const child = block.find((line) => line.trim() !== '' && !line.trim().startsWith('#'));
    const indent = /^(\s+)/.exec(child ?? '')?.[1] ?? '  ';
    lines.splice(start + 1, 0, entryLine(indent, value));
  }
  writeFileSync(file, lines.join('\n'));
  return { kind: 'set', file };
}
