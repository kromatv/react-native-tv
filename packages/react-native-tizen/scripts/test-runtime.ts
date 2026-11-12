import { createHash } from 'node:crypto';
import { mkdirSync, realpathSync, writeFileSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { parseArgs } from 'node:util';
import { loadAppProject } from '../src/app/app-project.ts';
import { cacheDir } from '../src/cache-dir.ts';
import { Reporter } from '../src/cli/reporter.ts';
import { FixableError } from '../src/errors.ts';
import { PACKAGE_ROOT, RUNTIME_DIR } from '../src/package-paths.ts';
import { ensureCodegen } from '../src/runtime/build-runtime.ts';
import { sourcesKey } from '../src/runtime/cmake.ts';
import { ninjaProgress } from '../src/runtime/ninja-progress.ts';
import { guestRun } from '../src/simulator/lima.ts';
import type { Task } from '../src/task.ts';
import { pathInside } from './path-inside.ts';
import { lcovToSonar } from './sonar-coverage.ts';

const TESTS = 'rn-tizen-host-tests';
const RUNTIME_LIBRARY = 'react-native-tizen';
const RUNTIME_LIBRARY_FILE = 'libreact-native-tizen.so';

type Runner = (
  task: Task,
  command: readonly string[],
  env?: Readonly<Record<string, string>>,
  onLine?: (line: string) => void,
) => Promise<string>;

interface TestBuild {
  readonly dir: string;
  readonly hermesCompilers: string;
  readonly codegen: string;
  readonly reactNativeDir: string;
  readonly expoModulesCoreDir: string;
  readonly expoModulesJsiDir: string;
  readonly coverage: boolean;
}

const runHere: Runner = (task, command, env, onLine) =>
  task.run(command, { env: { ...process.env, ...env }, onLine });
const runGuest: Runner = process.platform === 'linux' ? runHere : guestRun;

function definitions(build: TestBuild): string[] {
  return [
    '-DRN_TIZEN_SIMULATOR=ON',
    '-DRN_TIZEN_TESTS=ON',
    `-DRN_TIZEN_COVERAGE=${build.coverage ? 'ON' : 'OFF'}`,
    '-DCMAKE_BUILD_TYPE=RelWithDebInfo',
    '-DCMAKE_C_COMPILER=clang',
    '-DCMAKE_CXX_COMPILER=clang++',
    '-DCMAKE_LINKER_TYPE=LLD',
    `-DRN_TIZEN_CACHE_DIR=${cacheDir()}`,
    `-DREACT_NATIVE_DIR=${build.reactNativeDir}`,
    `-DRN_TIZEN_CODEGEN_DIR=${build.codegen}`,
    `-DRN_TIZEN_HERMES_HOST_COMPILERS=${build.hermesCompilers}`,
    `-DEXPO_MODULES_CORE_DIR=${build.expoModulesCoreDir}`,
    `-DEXPO_MODULES_JSI_DIR=${build.expoModulesJsiDir}`,
  ];
}

async function requireHermesCompilers(task: Task, build: TestBuild): Promise<void> {
  const script = 'test -f "$1" && echo yes || true';
  const present = await runGuest(task, ['bash', '-c', script, 'env', build.hermesCompilers]);
  if (present.trim() === 'yes') return;
  throw new FixableError(
    'Hermes has no compiler for the simulator yet',
    'rn-tizen simulator build',
  );
}

async function compileTests(task: Task, build: TestBuild): Promise<void> {
  task.progress('configuring');
  const configure = ['cmake', '-S', RUNTIME_DIR, '-B', build.dir, '-G', 'Ninja'];
  await runGuest(task, [...configure, ...definitions(build)]);
  const targets = build.coverage ? [TESTS, RUNTIME_LIBRARY] : [TESTS];
  const compile = ['cmake', '--build', build.dir, '--target', ...targets];
  const ccache = { CCACHE_BASEDIR: build.dir, CCACHE_NOHASHDIR: '1' };
  await runGuest(task, compile, ccache, (line) => {
    const progress = ninjaProgress(line);
    if (progress !== null) task.progress(progress);
  });
}

async function runTests(task: Task, build: TestBuild, profile: string): Promise<string> {
  task.progress('running');
  await runGuest(task, ['rm', '-f', profile]);
  const ctest = ['ctest', '--test-dir', build.dir, '--output-on-failure', '--verbose'];
  return runGuest(task, ctest, { LLVM_PROFILE_FILE: profile });
}

async function exportCoverage(task: Task, build: TestBuild, profile: string): Promise<string> {
  const profdata = join(build.dir, 'tests.profdata');
  await runGuest(task, ['llvm-profdata', 'merge', '-sparse', profile, '-o', profdata]);
  return runGuest(task, [
    'llvm-cov',
    'export',
    '-format=lcov',
    `-instr-profile=${profdata}`,
    join(build.dir, 'host', TESTS),
    `-object=${join(build.dir, 'host', RUNTIME_LIBRARY_FILE)}`,
  ]);
}

function doctestSummary(output: string): string {
  return /test cases:[^\n]*/.exec(output)?.[0].replaceAll(/\s+/g, ' ') ?? 'passed';
}

const { values, positionals } = parseArgs({
  allowPositionals: true,
  options: { coverage: { type: 'string' } },
});
const app = loadAppProject(resolve(positionals[0] ?? 'example'));
const reporter = new Reporter({ verbose: process.env.CI === 'true' });
const { reactNative, expoModulesCore, expoModulesJsi } = app.packages;
const runtimeName = `${reactNative.name}-${reactNative.version}-simulator-${sourcesKey(RUNTIME_DIR, reactNative.dir)}`;
const codegen = join(cacheDir(), 'runtime', runtimeName, 'codegen');
const worktree = createHash('sha256').update(realpathSync(PACKAGE_ROOT)).digest('hex').slice(0, 12);

const buildRoot = await reporter.step('Codegen', async (task) => {
  await ensureCodegen(reactNative, codegen);
  const script = 'source "$1/guest/env.sh" && echo "$BUILD_DIR"';
  const output = await runGuest(task, [
    'bash',
    '-c',
    script,
    'env',
    join(PACKAGE_ROOT, 'simulator'),
  ]);
  return output.trim();
});
const build: TestBuild = {
  dir: join(buildRoot, 'host-tests', worktree),
  hermesCompilers: join(buildRoot, 'hermes-host', 'ImportHostCompilers.cmake'),
  codegen,
  reactNativeDir: realpathSync(reactNative.dir),
  expoModulesCoreDir: realpathSync(expoModulesCore.dir),
  expoModulesJsiDir: realpathSync(expoModulesJsi.dir),
  coverage: values.coverage !== undefined,
};
const report =
  values.coverage === undefined ? null : pathInside(values.coverage, [PACKAGE_ROOT, build.dir]);
const profile = join(build.dir, 'tests.profraw');

await reporter.step(
  'Host tests',
  async (task) => {
    await requireHermesCompilers(task, build);
    await compileTests(task, build);
    return runTests(task, build, profile);
  },
  doctestSummary,
);

if (report !== null) {
  await reporter.step(
    'Coverage',
    async (task) => {
      const lcov = await exportCoverage(task, build, profile);
      const coverage = lcovToSonar(lcov, PACKAGE_ROOT, (path) => path.startsWith('runtime/'));
      mkdirSync(dirname(report), { recursive: true });
      writeFileSync(report, coverage.xml);
      return coverage.files;
    },
    (files) => `${files} files, ${report}`,
  );
}
