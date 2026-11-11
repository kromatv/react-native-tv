import { existsSync, mkdtempSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { findPackage } from '../app/app-packages.ts';
import { generateReactNativeSpecs } from './react-native-specs.ts';

let output: string;

beforeEach(() => {
  output = mkdtempSync(join(tmpdir(), 'rn-tizen-rn-specs-'));
});

afterEach(() => {
  rmSync(output, { recursive: true, force: true });
});

describe('generateReactNativeSpecs', () => {
  it("writes React Native's TurboModule bases and core components where ReactCommon includes them", async () => {
    const reactNative = findPackage(process.cwd(), 'react-native');
    if (reactNative === null) throw new Error('react-native is not installed');

    const specs = await generateReactNativeSpecs(reactNative.dir, output);

    expect(specs.includeDir).toBe(output);
    expect(existsSync(join(output, 'FBReactNativeSpec', 'FBReactNativeSpecJSI.h'))).toBe(true);
    expect(specs.sources.length).toBeGreaterThan(0);
    expect(specs.sources.every((source) => source.endsWith('.cpp'))).toBe(true);
  });
});
