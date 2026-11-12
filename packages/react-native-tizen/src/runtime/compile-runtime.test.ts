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
vi.mock('../codegen/react-native-specs.ts', () => ({
  generateReactNativeSpecs: vi.fn(async () => ({ includeDir: '', sources: [] })),
}));

const SDK = { root: '/tizen-studio', sdb: '/sdb', tizenCli: '/tizen', packageManager: '/pm' };
const TARGET = { arch: 'armv7l', platformVersion: '9.0' } as const;
let dir: string;

function inputs() {
  const app = fakeApp(dir);
  return {
    sdk: SDK,
    target: TARGET,
    flavour: 'development' as const,
    packages: app.packages,
    hermes: { version: '0.1', importFile: '/hermes/Import.cmake', hermesc: '/hermes/hermesc' },
    dirs: runtimeBuildDirs(app.packages.reactNative, TARGET, 'development'),
    nativeLibrariesFile: '/libraries.json',
  };
}

beforeEach(() => {
  dir = mkdtempSync(join(tmpdir(), 'rn-tizen-compile-'));
  vi.stubEnv('RN_TIZEN_CACHE_DIR', join(dir, 'cache'));
});

afterEach(() => {
  rmSync(dir, { recursive: true, force: true });
  vi.clearAllMocks();
  vi.unstubAllEnvs();
});

describe('compileRuntime', () => {
  it("builds the runtime target against the app's packages and returns the library", async () => {
    const build = inputs();
    vi.mocked(buildCMakeProject).mockImplementation(async () => {
      mkdirSync(dirname(build.dirs.library), { recursive: true });
      writeFileSync(build.dirs.library, '');
    });

    expect(await compileRuntime(recordingTask(), build)).toBe(build.dirs.library);
    expect(buildCMakeProject).toHaveBeenCalledWith(
      expect.anything(),
      expect.objectContaining({
        targets: ['react-native-tizen'],
        definitions: expect.objectContaining({ RN_TIZEN_LIBRARIES_FILE: '/libraries.json' }),
      }),
    );
  });

  it('compiles the dev server tooling into the development runtime only', async () => {
    const build = inputs();
    vi.mocked(buildCMakeProject).mockImplementation(async () => {
      mkdirSync(dirname(build.dirs.library), { recursive: true });
      writeFileSync(build.dirs.library, '');
    });

    await compileRuntime(recordingTask(), build);
    await compileRuntime(recordingTask(), { ...build, flavour: 'release' });

    const flavours = vi
      .mocked(buildCMakeProject)
      .mock.calls.map(([, project]) => project.definitions['RN_TIZEN_DEVELOPMENT']);
    expect(flavours).toEqual(['ON', 'OFF']);
  });

  it('says to look at the build output when the library is missing after it', async () => {
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
