import { readFileSync } from 'node:fs';
import { join } from 'node:path';
import { describe, expect, it } from 'vitest';
import { PACKAGE_ROOT } from '../package-paths.ts';
import { isReactNativeTvosSpec, REACT_NATIVE_TVOS_SPEC } from './react-native-tvos.ts';

describe('REACT_NATIVE_TVOS_SPEC', () => {
  it('is the react-native-tvos this repository builds and tests against', () => {
    const manifest = JSON.parse(readFileSync(join(PACKAGE_ROOT, 'package.json'), 'utf8'));

    expect(manifest.devDependencies['react-native']).toBe(REACT_NATIVE_TVOS_SPEC);
    expect(isReactNativeTvosSpec(REACT_NATIVE_TVOS_SPEC)).toBe(true);
    expect(isReactNativeTvosSpec('0.88.1')).toBe(false);
  });
});
