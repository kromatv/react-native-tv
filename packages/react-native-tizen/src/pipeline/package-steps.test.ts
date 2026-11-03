import { existsSync, mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { fakeApp } from '../app/app.fixtures.ts';
import { bundleApp } from '../bundle/bundle-app.ts';
import { Reporter } from '../cli/reporter.ts';
import { capturedOutput } from '../fakes.fixtures.ts';
import { buildDotnetProject } from '../package/dotnet-build.ts';
import { writeTizenProject } from '../package/tizen-project.ts';
import { ensureIcuData } from '../runtime/icu-data.ts';
import { signPackage } from '../signing/sign-package.ts';
import { testSigner } from '../signing/signing.fixtures.ts';
import { platformProfile } from '../tizen/platform-profiles.ts';
import { resolveSigning, type ResolvedSigning } from '../tizen/signing-identity.ts';
import { bundleDir, bundleStep, packageSteps } from './package-steps.ts';
import { runtimeSteps } from './runtime-steps.ts';

vi.mock('../bundle/bundle-app.ts', () => ({ bundleApp: vi.fn() }));
vi.mock('../runtime/icu-data.ts', () => ({ ensureIcuData: vi.fn() }));
vi.mock('../package/tizen-project.ts', () => ({ writeTizenProject: vi.fn() }));
vi.mock('../package/dotnet-build.ts', () => ({ buildDotnetProject: vi.fn() }));
vi.mock('../tizen/signing-identity.ts', () => ({ resolveSigning: vi.fn() }));
vi.mock('../signing/sign-package.ts', () => ({ signPackage: vi.fn() }));
vi.mock('./runtime-steps.ts', () => ({ runtimeSteps: vi.fn() }));

const SDK = { root: '/tizen-studio', packageManager: '/pm' };
const signer = testSigner('Store');
const [certificate] = signer.chain;
if (certificate === undefined) throw new Error('the test signer has no certificate');
const key = { chain: signer.chain, sign: () => Buffer.alloc(0) };
const SIGNING: ResolvedSigning = {
  name: 'Store',
  identity: { author: key, distributors: [key] },
  distributor: certificate,
};

let root: string;

function file(name: string): string {
  const path = join(root, name);
  writeFileSync(path, name);
  return path;
}

function reporter() {
  return new Reporter({ verbose: false, out: capturedOutput(), env: { NO_COLOR: '1' } });
}

beforeEach(() => {
  root = mkdtempSync(join(tmpdir(), 'rn-tizen-steps-'));
  const bundle = { javascript: file('index.js'), bytecode: file('index.hbc'), assetsDir: root };
  vi.mocked(bundleApp).mockResolvedValue(bundle);
  vi.mocked(ensureIcuData).mockResolvedValue(file('icudt77l.dat'));
  vi.mocked(runtimeSteps).mockResolvedValue(file('libreact-native-tizen.so'));
  vi.mocked(writeTizenProject).mockResolvedValue(join(root, 'tizen'));
  vi.mocked(buildDotnetProject).mockResolvedValue(join(root, 'app.tpk'));
  vi.mocked(resolveSigning).mockResolvedValue(SIGNING);
});

afterEach(() => {
  rmSync(root, { recursive: true, force: true });
  vi.clearAllMocks();
});

describe('bundleStep', () => {
  it("bundles into the app's tizen/ by default, claiming it first", async () => {
    const app = fakeApp(root);

    await bundleStep(reporter(), app);

    expect(bundleApp).toHaveBeenCalledWith(expect.anything(), app, bundleDir(app));
    expect(existsSync(join(app.tizenDir, '.gitignore'))).toBe(true);
  });

  it('bundles into the directory given, and leaves tizen/ alone', async () => {
    const app = fakeApp(root);

    await bundleStep(reporter(), app, join(root, 'dist'));

    expect(bundleApp).toHaveBeenCalledWith(expect.anything(), app, join(root, 'dist'));
    expect(existsSync(app.tizenDir)).toBe(false);
  });
});

describe('packageSteps', () => {
  it('builds, bundles, generates, compiles and signs with the certificates it settled', async () => {
    const app = fakeApp(root);

    const tpk = await packageSteps(reporter(), {
      sdk: SDK,
      app,
      target: { arch: 'armv7l', platformVersion: '9.0' },
      shell: platformProfile('9.0'),
      log: null,
      signing: { author: 'author.p12' },
    });

    expect(tpk).toBe(join(root, 'app.tpk'));
    expect(resolveSigning).toHaveBeenCalledWith({
      author: 'author.p12',
      profile: app.config.profile,
    });
    expect(writeTizenProject).toHaveBeenCalledWith(
      expect.objectContaining({ runtimeLibrary: join(root, 'libreact-native-tizen.so') }),
    );
    expect(signPackage).toHaveBeenCalledWith(tpk, SIGNING.identity);
  });

  it("stops before compiling anything when the app's tizen/ is its own", async () => {
    const app = fakeApp(root);
    writeFileSync(join(root, 'tizen'), 'mine');

    await expect(
      packageSteps(reporter(), {
        sdk: SDK,
        app,
        target: { arch: 'armv7l', platformVersion: '9.0' },
        shell: platformProfile('9.0'),
        log: null,
        signing: { profile: 'Store' },
      }),
    ).rejects.toThrow("is the app's own");
    expect(resolveSigning).not.toHaveBeenCalled();
    expect(runtimeSteps).not.toHaveBeenCalled();
  });
});
