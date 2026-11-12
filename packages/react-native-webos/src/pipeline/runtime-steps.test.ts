import { mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { fakeApp } from '../app/app.fixtures.ts';
import { Reporter } from '../cli/reporter.ts';
import { capturedOutput } from '../fakes.fixtures.ts';
import { prepareNativeLibraries } from '../libraries/prepare-native-libraries.ts';
import { compileRuntime, ensureCodegen } from '../runtime/build-runtime.ts';
import { ensureWebOSSdk } from '../sysroot/webos-sdk.ts';
import { runtimeSteps } from './runtime-steps.ts';

vi.mock('../sysroot/webos-sdk.ts', () => ({ ensureWebOSSdk: vi.fn() }));
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

const SDK = { path: '/cache/sdk/2026.08-webos', version: '2026.08-webos', reused: false };
let dir: string;
let out: ReturnType<typeof capturedOutput>;

beforeEach(() => {
  dir = mkdtempSync(join(tmpdir(), 'rn-webos-runtime-steps-'));
  vi.stubEnv('RN_WEBOS_CACHE_DIR', dir);
  out = capturedOutput();
  const executable = join(dir, 'react-native-webos');
  writeFileSync(executable, 'elf');
  vi.mocked(ensureWebOSSdk).mockResolvedValue(SDK);
  vi.mocked(ensureCodegen).mockResolvedValue(true);
  vi.mocked(prepareNativeLibraries).mockResolvedValue({
    file: '/libraries.json',
    libraries: [],
    warnings: ['maps: its codegenConfig is not one React Native reads; skipped'],
  });
  vi.mocked(compileRuntime).mockResolvedValue(executable);
});

afterEach(() => {
  rmSync(dir, { recursive: true, force: true });
  vi.clearAllMocks();
  vi.unstubAllEnvs();
});

describe('runtimeSteps', () => {
  it('prepares the SDK, codegen and libraries, then compiles the runtime', async () => {
    const reporter = new Reporter({ verbose: false, out, env: { NO_COLOR: '1' } });

    const executable = await runtimeSteps(reporter, fakeApp(dir), 'development');

    expect(executable).toBe(join(dir, 'react-native-webos'));
    expect(compileRuntime).toHaveBeenCalledWith(
      expect.anything(),
      expect.objectContaining({
        sdk: SDK,
        flavour: 'development',
        nativeLibrariesFile: '/libraries.json',
      }),
    );
    expect(out.text()).toContain('SDK 2026.08-webos extracted');
    expect(out.text()).toContain('react-native 0.88.0');
    expect(out.text()).toContain('maps: its codegenConfig');
  });

  it('says so when the SDK was already in the cache', async () => {
    vi.mocked(ensureWebOSSdk).mockResolvedValue({ ...SDK, reused: true });

    await runtimeSteps(
      new Reporter({ verbose: false, out, env: { NO_COLOR: '1' } }),
      fakeApp(dir),
      'release',
    );

    expect(out.text()).toMatch(/Toolchain\s+cached/);
  });
});
