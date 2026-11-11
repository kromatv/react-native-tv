import { mkdirSync, mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { dirname, join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { fakeApp } from '../app/app.fixtures.ts';
import { FixableError } from '../errors.ts';
import { recordingTask } from '../fakes.fixtures.ts';
import { compileRuntime, ensureCodegen, runtimeBuildDirs } from './build-runtime.ts';
import { buildCMakeProject } from './cmake.ts';
import { generateReactNativeSpecs } from '../codegen/react-native-specs.ts';

vi.mock('./cmake.ts', async (importOriginal) => ({
  ...(await importOriginal<typeof import('./cmake.ts')>()),
  buildCMakeProject: vi.fn(),
}));
vi.mock('./llvm-dir.ts', () => ({ locateLlvmDir: () => '/llvm/bin' }));
vi.mock('../codegen/react-native-specs.ts', () => ({
  generateReactNativeSpecs: vi.fn(async () => ({ includeDir: '', sources: [] })),
}));

const SDK = { path: '/cache/sdk', version: '2026.08-webos', reused: true };
let dir: string;

function inputs() {
  const app = fakeApp(dir);
  return {
    sdk: SDK,
    flavour: 'development' as const,
    packages: app.packages,
    hermes: { version: '0.1', importFile: '/hermes/Import.cmake', hermesc: '/hermes/hermesc' },
    dirs: runtimeBuildDirs(app.packages.reactNative, 'development'),
    nativeLibrariesFile: '/libraries.json',
  };
}

beforeEach(() => {
  dir = mkdtempSync(join(tmpdir(), 'rn-webos-compile-'));
  vi.stubEnv('RN_WEBOS_CACHE_DIR', join(dir, 'cache'));
});

afterEach(() => {
  rmSync(dir, { recursive: true, force: true });
  vi.clearAllMocks();
  vi.unstubAllEnvs();
});

describe('compileRuntime', () => {
  it("builds the runtime target against the app's packages and returns the executable", async () => {
    const build = inputs();
    vi.mocked(buildCMakeProject).mockImplementation(async () => {
      mkdirSync(dirname(build.dirs.executable), { recursive: true });
      writeFileSync(build.dirs.executable, '');
    });

    expect(await compileRuntime(recordingTask(), build)).toBe(build.dirs.executable);
    expect(buildCMakeProject).toHaveBeenCalledWith(
      expect.anything(),
      expect.objectContaining({
        targets: ['react-native-webos'],
        definitions: expect.objectContaining({
          RN_WEBOS_LIBRARIES_FILE: '/libraries.json',
          WEBOS_SDK_DIR: '/cache/sdk',
          WEBOS_LLVM_DIR: '/llvm/bin',
        }),
      }),
    );
  });

  it('compiles the dev server tooling into the development runtime only', async () => {
    const build = inputs();
    vi.mocked(buildCMakeProject).mockImplementation(async () => {
      mkdirSync(dirname(build.dirs.executable), { recursive: true });
      writeFileSync(build.dirs.executable, '');
    });

    await compileRuntime(recordingTask(), build);
    await compileRuntime(recordingTask(), { ...build, flavour: 'release' });

    const flavours = vi
      .mocked(buildCMakeProject)
      .mock.calls.map(([, project]) => project.definitions['RN_WEBOS_DEVELOPMENT']);
    expect(flavours).toEqual(['ON', 'OFF']);
  });

  it('says to look at the build output when the executable is missing after it', async () => {
    await expect(compileRuntime(recordingTask(), inputs())).rejects.toBeInstanceOf(FixableError);
  });
});

describe('ensureCodegen', () => {
  it('runs codegen once for the same react-native', async () => {
    const { reactNative } = fakeApp(dir).packages;
    const output = join(dir, 'codegen');
    mkdirSync(output);

    expect(await ensureCodegen(reactNative, output)).toBe(true);
    expect(await ensureCodegen(reactNative, output)).toBe(false);
    expect(generateReactNativeSpecs).toHaveBeenCalledTimes(1);
  });
});
