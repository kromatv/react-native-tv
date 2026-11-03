import { mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { fakeApp } from '../app/app.fixtures.ts';
import { Reporter } from '../cli/reporter.ts';
import { capturedOutput } from '../fakes.fixtures.ts';
import { prepareNativeLibraries } from '../libraries/prepare-native-libraries.ts';
import { compileRuntime, ensureCodegen } from '../runtime/build-runtime.ts';
import { setupTarget } from '../sysroot/setup-target.ts';
import { runtimeSteps } from './runtime-steps.ts';

vi.mock('../sysroot/setup-target.ts', () => ({ setupTarget: vi.fn() }));
vi.mock('../runtime/hermes-host.ts', () => ({
  hermesHostCompilers: () => ({ built: true }),
  ensureHermesHostCompilers: vi.fn(async () => ({
    version: '0.1',
    importFile: '/i',
    hermesc: '/h',
  })),
}));
vi.mock('../runtime/build-runtime.ts', async (importOriginal) => ({
  ...(await importOriginal<typeof import('../runtime/build-runtime.ts')>()),
  ensureCodegen: vi.fn(),
  compileRuntime: vi.fn(),
}));
vi.mock('../libraries/prepare-native-libraries.ts', async (importOriginal) => ({
  ...(await importOriginal<typeof import('../libraries/prepare-native-libraries.ts')>()),
  prepareNativeLibraries: vi.fn(),
}));

const SDK = { root: '/tizen-studio', sdb: '/sdb', tizenCli: '/tizen', packageManager: '/pm' };
let dir: string;
let out: ReturnType<typeof capturedOutput>;

beforeEach(() => {
  dir = mkdtempSync(join(tmpdir(), 'rn-tizen-runtime-steps-'));
  vi.stubEnv('RN_TIZEN_CACHE_DIR', dir);
  out = capturedOutput();
  const library = join(dir, 'libreact-native-tizen.so');
  writeFileSync(library, 'elf');
  vi.mocked(setupTarget).mockResolvedValue({
    path: '/sysroot',
    reused: false,
    patchedLibraries: 3,
  });
  vi.mocked(ensureCodegen).mockResolvedValue(true);
  vi.mocked(prepareNativeLibraries).mockResolvedValue({
    file: '/libraries.json',
    libraries: [],
    warnings: ['maps: its codegenConfig is not one React Native reads; skipped'],
  });
  vi.mocked(compileRuntime).mockResolvedValue(library);
});

afterEach(() => {
  rmSync(dir, { recursive: true, force: true });
  vi.clearAllMocks();
  vi.unstubAllEnvs();
});

describe('runtimeSteps', () => {
  it('prepares the toolchain, codegen and libraries, then compiles the runtime', async () => {
    const reporter = new Reporter({ verbose: false, out, env: { NO_COLOR: '1' } });

    const library = await runtimeSteps(reporter, SDK, fakeApp(dir), {
      arch: 'armv7l',
      platformVersion: '9.0',
    });

    expect(library).toBe(join(dir, 'libreact-native-tizen.so'));
    expect(compileRuntime).toHaveBeenCalledWith(
      expect.anything(),
      expect.objectContaining({ sdk: SDK, nativeLibrariesFile: '/libraries.json' }),
    );
    expect(out.text()).toContain('sysroot mirrored, 3 libraries patched');
    expect(out.text()).toContain('react-native 0.88.0');
    expect(out.text()).toContain('maps: its codegenConfig');
  });
});
