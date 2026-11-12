import { accessSync, constants } from 'node:fs';
import { delimiter, join } from 'node:path';

const LLVM_HINTS = ['/opt/homebrew/opt/llvm/bin', '/usr/local/opt/llvm/bin'];
const LLD_HINTS = [
  '/opt/homebrew/opt/lld/bin',
  '/usr/local/opt/lld/bin',
  '/opt/homebrew/opt/llvm/bin',
];

function isExecutable(path: string): boolean {
  try {
    accessSync(path, constants.X_OK);
    return true;
  } catch {
    return false;
  }
}

/** The first executable named `name` in `dirs`, then on PATH. */
export function findProgram(
  name: string,
  dirs: readonly string[] = [],
  env: NodeJS.ProcessEnv = process.env,
): string | null {
  const path = (env.PATH ?? '').split(delimiter).filter((dir) => dir !== '');
  for (const dir of [...dirs, ...path]) {
    const candidate = join(dir, name);
    if (isExecutable(candidate)) return candidate;
  }
  return null;
}

/** The LLVM tools the Tizen toolchain file looks for, searched the way it searches. */
export function findLlvmTools(env: NodeJS.ProcessEnv = process.env) {
  const own = env.TIZEN_LLVM_DIR === undefined ? [] : [env.TIZEN_LLVM_DIR];
  const llvm = [...own, ...LLVM_HINTS];
  return {
    clang: findProgram('clang', llvm, env),
    'clang++': findProgram('clang++', llvm, env),
    'llvm-ar': findProgram('llvm-ar', llvm, env),
    'llvm-ranlib': findProgram('llvm-ranlib', llvm, env),
    'llvm-strip': findProgram('llvm-strip', llvm, env),
    'llvm-symbolizer': findProgram('llvm-symbolizer', llvm, env),
    'ld.lld': findProgram('ld.lld', [...own, ...LLD_HINTS], env),
  };
}
