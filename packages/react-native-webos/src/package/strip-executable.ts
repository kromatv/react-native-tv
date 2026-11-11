import { findLlvmTools } from '../doctor/find-program.ts';
import { FixableError } from '../errors.ts';
import { runSync } from '../exec.ts';

const STRIP_TIMEOUT_MS = 60_000;

/**
 * Removes the symbol table and debug information of the executable at `path`,
 * in place. A TV never reads them: the dynamic linker uses the dynamic symbols
 * and unwinding uses its own tables, both of which stay.
 */
export function stripExecutable(path: string): void {
  const strip = findLlvmTools()['llvm-strip'];
  if (strip === null) {
    throw new FixableError(
      'llvm-strip was not found',
      'Install LLVM (`brew install llvm`), or set WEBOS_LLVM_DIR to the directory that holds it.',
    );
  }
  runSync([strip, '--strip-all', path], { timeoutMs: STRIP_TIMEOUT_MS });
}
