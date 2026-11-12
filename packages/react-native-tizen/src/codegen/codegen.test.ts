import { mkdtempSync, readdirSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { findPackage } from '../app/app-packages.ts';
import { FixableError } from '../errors.ts';
import { loadCodegen, MODULE_GENERATORS } from './codegen.ts';

const SPEC = `import type { TurboModule } from 'react-native';
import { TurboModuleRegistry } from 'react-native';

export interface Spec extends TurboModule {
  greet(name: string): string;
}

export default TurboModuleRegistry.getEnforcing<Spec>('Greeter');
`;

let dir: string;

function reactNativeDir(): string {
  const reactNative = findPackage(process.cwd(), 'react-native');
  if (reactNative === null) throw new Error('react-native is not installed');
  return reactNative.dir;
}

beforeEach(() => {
  dir = mkdtempSync(join(tmpdir(), 'rn-tizen-codegen-'));
});

afterEach(() => {
  rmSync(dir, { recursive: true, force: true });
});

describe('loadCodegen', () => {
  it("reads a TurboModule spec and writes its C++ base with react-native's own codegen", async () => {
    writeFileSync(join(dir, 'NativeGreeter.ts'), SPEC);
    const codegen = await loadCodegen(reactNativeDir());

    const schema = codegen.schema([dir], 'GreeterSpec');
    const sources = await codegen.write(MODULE_GENERATORS, 'GreeterSpec', schema, join(dir, 'out'));

    expect(readdirSync(join(dir, 'out'))).toContain('GreeterSpecJSI.h');
    expect(sources).toEqual([]);
  });

  it('fails on a generator the installed codegen does not have', async () => {
    const codegen = await loadCodegen(reactNativeDir());

    await expect(codegen.write(['generateNothing'], 'Spec', {}, dir)).rejects.toThrow(
      '@react-native/codegen has no generateNothing',
    );
  });

  it('asks for the dependencies when react-native comes without its codegen', async () => {
    writeFileSync(join(dir, 'package.json'), '{}');

    await expect(loadCodegen(dir)).rejects.toBeInstanceOf(FixableError);
  });
});
