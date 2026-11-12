import { installHint } from '../cli/install-hints.ts';
import { missingLlvmTools } from '../runtime/llvm-dir.ts';
import { SDK_PIN } from '../sysroot/sdk-pin.ts';
import { installedWebOSSdk } from '../sysroot/webos-sdk.ts';
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

async function versionOf(task: Task, command: readonly string[]): Promise<string> {
  const output = await task.run(command);
  return parseVersion(output) ?? output.trim().split('\n')[0] ?? 'unknown';
}

/** The SDK a TV build compiles against, which the first build downloads; a warning until then. */
export function checkSdk(): Promise<CheckResult> {
  return checking('webOS SDK', async () => {
    const sdk = await installedWebOSSdk();
    if (sdk !== null) return passed('webOS SDK', `${sdk.version} at ${sdk.path}`);
    return failed(
      'webOS SDK',
      `${SDK_PIN.version} not downloaded yet`,
      'The first build downloads it; rn-webos setup does it ahead of time.',
      'warn',
    );
  });
}

/** Homebrew's (or `WEBOS_LLVM_DIR`'s) clang and lld, which cross-compile for webOS. */
export function checkLlvm(task: Task): Promise<CheckResult> {
  return checking('LLVM', async () => {
    const tools = findLlvmTools();
    const missing = missingLlvmTools();
    if (missing.length > 0 || tools.clang === null) {
      return failed(
        'LLVM',
        `missing ${missing.join(', ')}`,
        `${installHint('clang')}, or set WEBOS_LLVM_DIR`,
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
