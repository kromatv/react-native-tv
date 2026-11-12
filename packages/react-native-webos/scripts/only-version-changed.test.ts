import { describe, expect, it } from 'vitest';
import { onlyVersionChanged } from './only-version-changed.ts';

const manifest = (fields: Record<string, unknown>) => JSON.stringify(fields, null, 2);
const base = { name: 'pkg', version: '0.1.0-alpha.0', dependencies: { zod: '^4.6.7' } };

describe('onlyVersionChanged', () => {
  it('is true for a release bump', () => {
    expect(
      onlyVersionChanged(manifest(base), manifest({ ...base, version: '0.1.0-alpha.1' })),
    ).toBe(true);
  });

  it('is false when anything besides the version changes', () => {
    const bumped = { ...base, version: '0.1.0-alpha.1', dependencies: { zod: '^4.7.0' } };
    expect(onlyVersionChanged(manifest(base), manifest(bumped))).toBe(false);
    expect(onlyVersionChanged(manifest(base), manifest({ ...base, files: ['dist'] }))).toBe(false);
  });

  it('refuses text that is not a JSON object', () => {
    expect(() => onlyVersionChanged('[]', manifest(base))).toThrow();
    expect(() => onlyVersionChanged('{', manifest(base))).toThrow();
  });
});
