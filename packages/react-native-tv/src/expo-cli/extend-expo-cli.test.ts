import { realpathSync } from 'node:fs';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { testApp, type TestApp } from '../app.fixtures.ts';
import {
  extendExpoCli,
  runnablePlatforms,
  type ExpoCliHost,
  type ExpoCommandLoader,
} from './extend-expo-cli.ts';

let app: TestApp;
let script: (name: string) => string;

interface Recorded {
  readonly runs: (readonly [string, readonly string[]])[];
  readonly exits: number[];
  readonly warnings: string[];
}

function host(recorded: Recorded, status = 0): ExpoCliHost {
  return {
    projectRoot: app.root,
    run: (file, argv) => {
      recorded.runs.push([file, argv]);
      return status;
    },
    exit: (code) => recorded.exits.push(code),
    warn: (message) => recorded.warnings.push(message),
  };
}

function recorder(): Recorded {
  return { runs: [], exits: [], warnings: [] };
}

function expoCommand(calls: (string[] | undefined)[]): ExpoCommandLoader {
  return () =>
    Promise.resolve((argv) => {
      calls.push(argv);
      return 'expo';
    });
}

beforeEach(() => {
  app = testApp();
  script = (name) => join(realpathSync(app.root), 'node_modules', name, 'cli.js');
  app.dependOn('tizen', 'webos', 'metro-only');
  app.platformPackage('tizen', { platform: 'tizen', cli: 'rn-tizen' });
  app.platformPackage('webos', { platform: 'webos', cli: 'rn-webos' });
  app.platformPackage('metro-only', {});
});

afterEach(() => {
  vi.restoreAllMocks();
  app.remove();
});

describe('runnablePlatforms', () => {
  it('gives the command of each platform that names one, the first package of a platform winning', () => {
    app.dependOn('tizen', 'webos', 'metro-only', 'webos-fork', 'no-cli');
    app.platformPackage('webos-fork', { platform: 'webos', cli: 'rn-webos-fork' });
    app.platformPackage('no-cli', { platform: 'roku' });

    expect([...runnablePlatforms(app.root)]).toEqual([
      ['tizen', script('tizen')],
      ['webos', script('webos')],
    ]);
  });
});

describe('extendExpoCli', () => {
  it('adds run:<platform> for each platform, which runs its command and exits with its code', async () => {
    const commands: Record<string, ExpoCommandLoader> = {};
    const recorded = recorder();
    extendExpoCli(commands, host(recorded, 4));

    expect(Object.keys(commands)).toEqual(['run:tizen', 'run:webos']);
    const runWebos = await commands['run:webos']?.();
    runWebos?.(['--device', 'tv']);
    const runTizen = await commands['run:tizen']?.();
    runTizen?.();

    expect(recorded).toEqual({
      runs: [
        [script('webos'), ['--device', 'tv']],
        [script('tizen'), []],
      ],
      exits: [4, 4],
      warnings: [],
    });
  });

  it("keeps a command Expo has already, and leaves Expo's own commands alone", () => {
    const start = expoCommand([]);
    const runWebos = expoCommand([]);
    const commands: Record<string, ExpoCommandLoader> = { start, 'run:webos': runWebos };
    extendExpoCli(commands, host(recorder()));

    expect(commands.start).toBe(start);
    expect(commands['run:webos']).toBe(runWebos);
    expect(Object.keys(commands)).toEqual(['start', 'run:webos', 'run:tizen']);
  });

  it('makes run <platform> run the platform, and passes every other run to Expo', async () => {
    const expoRuns: (string[] | undefined)[] = [];
    const commands: Record<string, ExpoCommandLoader> = { run: expoCommand(expoRuns) };
    const recorded = recorder();
    extendExpoCli(commands, host(recorded));
    const run = await commands.run?.();

    run?.(['tizen', '--release']);
    expect(run?.(['android', '--device'])).toBe('expo');
    expect(run?.()).toBe('expo');

    expect(recorded.runs).toEqual([[script('tizen'), ['--release']]]);
    expect(recorded.exits).toEqual([0]);
    expect(expoRuns).toEqual([['android', '--device'], []]);
  });

  it("leaves Expo's run as it is in an app with no platform to run", () => {
    app.dependOn('metro-only');
    const run = expoCommand([]);
    const commands: Record<string, ExpoCommandLoader> = { run };
    extendExpoCli(commands, host(recorder()));

    expect(commands).toEqual({ run });
  });

  it('warns and adds nothing when Expo gives no command table', () => {
    const recorded = recorder();
    extendExpoCli({ start: 'not a loader' }, host(recorded));
    extendExpoCli(undefined, host(recorded));

    expect(recorded.warnings).toEqual([
      '@kromatv/react-native-tv could not add its commands: Expo CLI gave no command table',
      '@kromatv/react-native-tv could not add its commands: Expo CLI gave no command table',
    ]);
  });

  it('reads the app in the current directory, and adds nothing where there is none', () => {
    vi.spyOn(process, 'cwd').mockReturnValue(join(app.root, 'nowhere'));
    const commands: Record<string, ExpoCommandLoader> = {};
    extendExpoCli(commands);

    expect(commands).toEqual({});
  });
});
