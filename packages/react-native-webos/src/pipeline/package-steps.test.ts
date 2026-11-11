import { existsSync, mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { fakeApp } from '../app/app.fixtures.ts';
import { bundleApp } from '../bundle/bundle-app.ts';
import { Reporter } from '../cli/reporter.ts';
import { FixableError } from '../errors.ts';
import { singleReactNativeIssue } from '../dependencies/react-native-copies.ts';
import { capturedOutput } from '../fakes.fixtures.ts';
import { webosMetroIssue } from '../metro/metro-platforms.ts';
import { buildDotnetProject } from '../package/dotnet-build.ts';
import { checkPackagedRuntime } from '../package/packaged-runtime.ts';
import { writeWebOSProject } from '../package/webos-project.ts';
import { ensureIcuData } from '../runtime/icu-data.ts';
import { signPackage } from '../signing/sign-package.ts';
import { testSigner } from '../signing/signing.fixtures.ts';
import { platformProfile } from '../webos/platform-profiles.ts';
import { resolveSigning, type ResolvedSigning } from '../webos/signing-identity.ts';
import { bundleDir, bundleStep, packageSteps } from './package-steps.ts';
import { runtimeSteps } from './runtime-steps.ts';

vi.mock('../bundle/bundle-app.ts', () => ({
  bundleApp: vi.fn(),
  BYTECODE_FILE: 'index.webos.hbc',
}));
vi.mock('../runtime/icu-data.ts', () => ({ ensureIcuData: vi.fn() }));
vi.mock('../package/webos-project.ts', () => ({ writeWebOSProject: vi.fn() }));
vi.mock('../package/dotnet-build.ts', () => ({ buildDotnetProject: vi.fn() }));
vi.mock('../webos/signing-identity.ts', () => ({ resolveSigning: vi.fn() }));
vi.mock('../signing/sign-package.ts', () => ({ signPackage: vi.fn() }));
vi.mock('./runtime-steps.ts', () => ({ runtimeSteps: vi.fn() }));
vi.mock('../metro/metro-platforms.ts', () => ({ webosMetroIssue: vi.fn(() => null) }));
vi.mock('../package/packaged-runtime.ts', () => ({ checkPackagedRuntime: vi.fn() }));
vi.mock('../dependencies/react-native-copies.ts', () => ({
  singleReactNativeIssue: vi.fn(() => null),
}));

const SDK = { root: '/webos-studio', packageManager: '/pm' };
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
  root = mkdtempSync(join(tmpdir(), 'rn-webos-steps-'));
  const bundle = { javascript: file('index.js'), bytecode: file('index.hbc'), assetsDir: root };
  vi.mocked(bundleApp).mockResolvedValue(bundle);
  vi.mocked(ensureIcuData).mockResolvedValue(file('icudt77l.dat'));
  vi.mocked(runtimeSteps).mockResolvedValue(file('libreact-native-webos.so'));
  vi.mocked(writeWebOSProject).mockResolvedValue(join(root, 'webos'));
  vi.mocked(buildDotnetProject).mockResolvedValue(join(root, 'app.tpk'));
  vi.mocked(resolveSigning).mockResolvedValue(SIGNING);
});

afterEach(() => {
  rmSync(root, { recursive: true, force: true });
  vi.clearAllMocks();
});

describe('bundleStep', () => {
  it("bundles into the app's webos/ by default, claiming it first", async () => {
    const app = fakeApp(root);

    await bundleStep(reporter(), app);

    expect(bundleApp).toHaveBeenCalledWith(expect.anything(), app, bundleDir(app));
    expect(existsSync(join(app.webosDir, '.gitignore'))).toBe(true);
  });

  it('bundles into the directory given, and leaves webos/ alone', async () => {
    const app = fakeApp(root);

    await bundleStep(reporter(), app, join(root, 'dist'));

    expect(bundleApp).toHaveBeenCalledWith(expect.anything(), app, join(root, 'dist'));
    expect(existsSync(app.webosDir)).toBe(false);
  });
});

describe('packageSteps', () => {
  it('builds, bundles, generates, compiles and signs with the certificates it settled', async () => {
    const app = fakeApp(root);

    const { tpk, distributor } = await packageSteps(reporter(), {
      sdk: SDK,
      app,
      target: { arch: 'armv7l', platformVersion: '9.0' },
      shell: platformProfile('9.0'),
      log: null,
      signing: { author: 'author.p12' },
    });

    expect(checkPackagedRuntime).toHaveBeenCalledWith(
      tpk,
      join(root, 'libreact-native-webos.so'),
      'release',
    );
    expect(tpk).toBe(join(root, 'app.tpk'));
    expect(distributor).toBe(SIGNING.distributor);
    expect(resolveSigning).toHaveBeenCalledWith({
      author: 'author.p12',
      profile: app.config.profile,
    });
    expect(writeWebOSProject).toHaveBeenCalledWith(
      expect.objectContaining({ runtimeLibrary: join(root, 'libreact-native-webos.so') }),
    );
    expect(signPackage).toHaveBeenCalledWith(tpk, SIGNING.identity);
    expect(vi.mocked(runtimeSteps).mock.calls[0]?.[4]).toBe('release');
  });

  it('points a development build at the dev server instead of bundling', async () => {
    const app = fakeApp(root);

    await packageSteps(reporter(), {
      sdk: SDK,
      app,
      target: { arch: 'armv7l', platformVersion: '9.0' },
      shell: platformProfile('9.0'),
      log: { host: '10.0.0.2', port: 8099 },
      devServer: 'http://localhost:8081',
      signing: { author: 'author.p12' },
    });

    expect(bundleApp).not.toHaveBeenCalled();
    expect(vi.mocked(runtimeSteps).mock.calls[0]?.[4]).toBe('development');
    const { bundle } = vi.mocked(writeWebOSProject).mock.calls[0]?.[0] ?? {};
    expect(bundle).toMatchObject({
      bundleUrl: expect.stringMatching(/^http:\/\/10\.0\.0\.2:8081\/\.expo\/.*app=/),
    });
  });

  it("stops before compiling anything when the app's webos/ is its own", async () => {
    const app = fakeApp(root);
    writeFileSync(join(root, 'webos'), 'mine');

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

  it.each([
    ['loads two react-natives', () => vi.mocked(singleReactNativeIssue), 'two copies'],
    ['Metro config has no withWebOS', () => vi.mocked(webosMetroIssue), 'no webos platform'],
  ])('stops before compiling anything when the app %s', async (_, check, message) => {
    check().mockReturnValueOnce(new FixableError(message, 'Run npx rn-webos init'));

    await expect(
      packageSteps(reporter(), {
        sdk: SDK,
        app: fakeApp(root),
        target: { arch: 'armv7l', platformVersion: '9.0' },
        shell: platformProfile('9.0'),
        log: null,
        signing: { profile: 'Store' },
      }),
    ).rejects.toThrow(message);
    expect(resolveSigning).not.toHaveBeenCalled();
    expect(runtimeSteps).not.toHaveBeenCalled();
  });
});
