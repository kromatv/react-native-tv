import { runInNewContext } from 'node:vm';
import { describe, expect, it } from 'vitest';
import { EXPO_CLI_ENTRY } from './expo-cli.fixtures.ts';
import { addExpoCliHook, EXPO_CLI_EXTENSION, hasExpoCliHook } from './expo-cli-hook.ts';

interface Loaded {
  readonly commands: Record<string, unknown>;
  readonly warnings: unknown[][];
  readonly resolved: unknown[][];
}

function load(source: string, extension: (() => unknown) | null): Loaded {
  const warnings: unknown[][] = [];
  const resolved: unknown[][] = [];
  const require = Object.assign(() => extension?.(), {
    resolve: (...request: unknown[]) => {
      resolved.push(request);
      if (extension === null) throw new Error('Cannot find module');
      return '/app/node_modules/tv/expo-cli.js';
    },
  });
  const sandbox = {
    require,
    process: { cwd: () => '/app' },
    console: { warn: (...message: unknown[]) => warnings.push(message) },
    _arg: () => ({ default: () => ({}) }),
    result: {},
  };
  runInNewContext(`${source}\nresult.commands = commands;`, sandbox);
  return { commands: Object(sandbox.result).commands, warnings, resolved };
}

describe('addExpoCliHook', () => {
  const patched = addExpoCliHook(EXPO_CLI_ENTRY) ?? '';

  it('puts the hook between the command table and the argument parser', () => {
    const hook = patched.indexOf(EXPO_CLI_EXTENSION);

    expect(hook).toBeGreaterThan(patched.indexOf('const commands = {'));
    expect(hook).toBeLessThan(patched.indexOf('const args = '));
    expect(patched.replace(/\{\n {4}let tvCommands[\s\S]*?\n\}\n(?=const args)/, '')).toBe(
      EXPO_CLI_ENTRY,
    );
  });

  it("gives the app's extension the command table", () => {
    const loaded = load(patched, () => ({
      extendExpoCli: (commands: Record<string, unknown>) => {
        commands['run:webos'] = 'added';
      },
    }));

    expect(Object.keys(loaded.commands)).toEqual(['start', 'run:webos']);
    expect(loaded.resolved).toEqual([[EXPO_CLI_EXTENSION, { paths: ['/app'] }]]);
    expect(loaded.warnings).toEqual([]);
  });

  it('does nothing, silently, in an app without the package', () => {
    const loaded = load(patched, null);

    expect(Object.keys(loaded.commands)).toEqual(['start']);
    expect(loaded.warnings).toEqual([]);
  });

  it("warns and keeps Expo's commands when the extension fails to load", () => {
    const failure = new Error('broken');
    const loaded = load(patched, () => {
      throw failure;
    });

    expect(Object.keys(loaded.commands)).toEqual(['start']);
    expect(loaded.warnings).toEqual([
      ['@kromatv/react-native-tv could not add its commands:', failure],
    ]);
  });

  it('gives null for a file with no argument parser, with two, or with its table after it', () => {
    const [table = '', parser = ''] = EXPO_CLI_ENTRY.split(/(?=const args)/);

    expect(addExpoCliHook(table)).toBeNull();
    expect(addExpoCliHook(table + parser + parser)).toBeNull();
    expect(addExpoCliHook(parser + table)).toBeNull();
    expect(addExpoCliHook(parser)).toBeNull();
  });
});

describe('hasExpoCliHook', () => {
  it('tells a patched entry from the one Expo ships', () => {
    expect(hasExpoCliHook(EXPO_CLI_ENTRY)).toBe(false);
    expect(hasExpoCliHook(addExpoCliHook(EXPO_CLI_ENTRY) ?? '')).toBe(true);
  });
});
