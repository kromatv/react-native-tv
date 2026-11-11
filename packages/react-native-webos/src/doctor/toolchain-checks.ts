import { installHint } from '../cli/install-hints.ts';
import type { Task } from '../task.ts';
import {
  checking,
  failed,
  isAtLeastVersion,
  parseVersion,
  passed,
  type CheckResult,
} from './check-result.ts';
import { findLlvmTools, findProgram } from './find-program.ts';

const MINIMUM_CMAKE = '3.24';
const MINIMUM_DOTNET = '6.0';

async function versionOf(task: Task, command: readonly string[]): Promise<string> {
  const output = await task.run(command);
  return parseVersion(output) ?? output.trim().split('\n')[0] ?? 'unknown';
}

/** Homebrew's (or `TIZEN_LLVM_DIR`'s) clang and lld, which cross-compile for Tizen. */
export function checkLlvm(task: Task): Promise<CheckResult> {
  return checking('LLVM', async () => {
    const tools = findLlvmTools();
    const missing = Object.entries(tools)
      .filter(([, path]) => path === null)
      .map(([name]) => name);
    if (missing.length > 0 || tools.clang === null) {
      return failed(
        'LLVM',
        `missing ${missing.join(', ')}`,
        `${installHint('clang')}, or set TIZEN_LLVM_DIR`,
      );
    }
    return passed('LLVM', `clang ${await versionOf(task, [tools.clang, '--version'])}, ld.lld`);
  });
}

/** CMake, new enough for the runtime's CMakeLists. */
export function checkCMake(task: Task): Promise<CheckResult> {
  return checking('CMake', async () => {
    const version = await versionOf(task, ['cmake', '--version']);
    if (isAtLeastVersion(version, MINIMUM_CMAKE)) return passed('CMake', version);
    return failed('CMake', `${version} is older than ${MINIMUM_CMAKE}`, installHint('cmake'));
  });
}

/** Ninja, the generator every native build uses. */
export function checkNinja(task: Task): Promise<CheckResult> {
  return checking('Ninja', async () =>
    passed('Ninja', await versionOf(task, ['ninja', '--version'])),
  );
}

/** The .NET SDK the shell is built with. */
export function checkDotnet(task: Task): Promise<CheckResult> {
  return checking('.NET SDK', async () => {
    const version = await versionOf(task, ['dotnet', '--version']);
    if (isAtLeastVersion(version, MINIMUM_DOTNET)) return passed('.NET SDK', version);
    return failed('.NET SDK', `${version} is older than ${MINIMUM_DOTNET}`, installHint('dotnet'));
  });
}

/** Node.js, which runs Expo's CLI for the bundle. */
export function checkNode(task: Task): Promise<CheckResult> {
  return checking('Node.js', async () =>
    passed('Node.js', await versionOf(task, ['node', '--version'])),
  );
}

/** ccache, optional, which the runtime build uses when it finds it. */
export function checkCcache(): Promise<CheckResult> {
  return checking('ccache', () => {
    if (findProgram('ccache') !== null) return passed('ccache', 'rebuilds reuse compiled objects');
    return failed(
      'ccache',
      'not installed (optional)',
      `Makes runtime rebuilds faster: ${installHint('ccache')}`,
      'warn',
    );
  });
}
