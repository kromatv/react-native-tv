import { existsSync, mkdirSync, mkdtempSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
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
import { elfHeader } from '../package/ipk.fixtures.ts';
import { readIpk } from '../package/read-ipk.ts';
import { ensureCaBundle } from '../runtime/ca-bundle.ts';
import { ensureIcuData } from '../runtime/icu-data.ts';
import { bundleDir, bundleStep, packageSteps } from './package-steps.ts';
import { runtimeSteps } from './runtime-steps.ts';

vi.mock('../bundle/bundle-app.ts', () => ({
  bundleApp: vi.fn(),
  BYTECODE_FILE: 'index.webos.hbc',
}));
vi.mock('../runtime/icu-data.ts', () => ({ ensureIcuData: vi.fn() }));
vi.mock('../runtime/ca-bundle.ts', () => ({ ensureCaBundle: vi.fn() }));
vi.mock('../package/app-config-file.ts', () => ({ writeAppConfig: vi.fn(async () => {}) }));
vi.mock('./runtime-steps.ts', () => ({ runtimeSteps: vi.fn() }));
vi.mock('../metro/metro-platforms.ts', () => ({ webosMetroIssue: vi.fn(() => null) }));
vi.mock('../dependencies/react-native-copies.ts', () => ({
  singleReactNativeIssue: vi.fn(() => null),
}));

let root: string;
let out: ReturnType<typeof capturedOutput>;

function file(name: string, contents: string | Uint8Array = name): string {
  const path = join(root, name);
  writeFileSync(path, contents);
  return path;
}

function reporter() {
  return new Reporter({ verbose: false, out, env: { NO_COLOR: '1' } });
}

beforeEach(() => {
  root = mkdtempSync(join(tmpdir(), 'rn-webos-steps-'));
  out = capturedOutput();
  const assetsDir = join(root, 'assets');
  mkdirSync(assetsDir);
  const bundle = { javascript: file('index.js'), bytecode: file('index.hbc'), assetsDir };
  vi.mocked(bundleApp).mockResolvedValue(bundle);
  vi.mocked(ensureIcuData).mockResolvedValue(file('icudt78l.dat'));
  vi.mocked(ensureCaBundle).mockResolvedValue(file('cacert.pem'));
  vi.mocked(runtimeSteps).mockResolvedValue(file('react-native-webos', elfHeader(40)));
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
  it('builds the runtime, bundles, stages the app directory and writes the ipk beside it', async () => {
    const app = fakeApp(join(root, 'project'));

    const { ipk, appId } = await packageSteps(reporter(), { app, log: null });

    expect(appId).toBe('org.example.demo');
    expect(ipk).toBe(join(app.webosDir, 'org.example.demo_1.0.0_arm.ipk'));
    expect(vi.mocked(runtimeSteps).mock.calls[0]?.slice(1)).toEqual([app, 'release']);
    const { control, dataEntries } = readIpk(readFileSync(ipk));
    expect(control.get('Package')).toBe('org.example.demo');
    expect(dataEntries.map((entry) => entry.path)).toEqual(
      expect.arrayContaining([
        'usr/palm/applications/org.example.demo/react-native-webos',
        'usr/palm/applications/org.example.demo/res/index.webos.hbc',
        'usr/palm/applications/org.example.demo/res/icudt78l.dat',
        'usr/palm/applications/org.example.demo/res/cacert.pem',
      ]),
    );
    expect(out.text()).toMatch(/Package\s+release runtime, installs/);
  });

  it('points a development build at the dev server instead of bundling', async () => {
    const app = fakeApp(join(root, 'project'));

    const { ipk } = await packageSteps(reporter(), {
      app,
      log: { host: '10.0.0.2', port: 8099 },
      devServer: 'http://localhost:8081',
    });

    expect(bundleApp).not.toHaveBeenCalled();
    expect(vi.mocked(runtimeSteps).mock.calls[0]?.[2]).toBe('development');
    const config = readIpk(readFileSync(ipk)).dataEntries.find((entry) =>
      entry.path.endsWith('res/react-native-webos.json'),
    );
    expect(JSON.parse(Buffer.from(config?.data ?? []).toString())).toMatchObject({
      development: true,
      log: { host: '10.0.0.2', port: 8099 },
      bundleUrl: expect.stringMatching(/^http:\/\/10\.0\.0\.2:8081\/\.expo\/.*app=/),
    });
  });

  it("stops before compiling anything when the app's webos/ is its own", async () => {
    const app = fakeApp(root);
    writeFileSync(join(root, 'webos'), 'mine');

    await expect(packageSteps(reporter(), { app, log: null })).rejects.toThrow("is the app's own");
    expect(runtimeSteps).not.toHaveBeenCalled();
  });

  it.each([
    ['loads two react-natives', () => vi.mocked(singleReactNativeIssue), 'two copies'],
    ['Metro config has no withWebOS', () => vi.mocked(webosMetroIssue), 'no webos platform'],
  ])('stops before compiling anything when the app %s', async (_, check, message) => {
    check().mockReturnValueOnce(new FixableError(message, 'Run npx rn-webos init'));

    await expect(packageSteps(reporter(), { app: fakeApp(root), log: null })).rejects.toThrow(
      message,
    );
    expect(runtimeSteps).not.toHaveBeenCalled();
  });
});
