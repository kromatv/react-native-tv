import { dirname } from 'node:path';
import { installHint } from '../cli/install-hints.ts';
import { findLlvmTools } from '../doctor/find-program.ts';
import { FixableError } from '../errors.ts';

const OPTIONAL_TOOLS = new Set(['llvm-symbolizer']);

/** The LLVM tools a TV build cannot do without that are not installed. */
export function missingLlvmTools(env: NodeJS.ProcessEnv = process.env): string[] {
  return Object.entries(findLlvmTools(env))
    .filter(([name, path]) => path === null && !OPTIONAL_TOOLS.has(name))
    .map(([name]) => name);
}

/**
 * The directory holding the clang a TV build compiles with, which the
 * toolchain file takes as `WEBOS_LLVM_DIR`: the one `WEBOS_LLVM_DIR` names,
 * else Homebrew's, else the one on PATH.
 */
export function locateLlvmDir(env: NodeJS.ProcessEnv = process.env): string {
  const { clang } = findLlvmTools(env);
  const missing = missingLlvmTools(env);
  if (clang !== null && missing.length === 0) return dirname(clang);
  throw new FixableError(
    `A TV build needs LLVM, and ${missing.join(', ')} ${missing.length === 1 ? 'is' : 'are'} missing`,
    `${installHint('clang')}, or set WEBOS_LLVM_DIR to the directory that holds clang and ld.lld.`,
  );
}
