import { describe, expect, it } from 'vitest';
import { painter } from '../paint.ts';
import { tvUsage } from './tv-usage.ts';

describe('tvUsage', () => {
  it("prints Expo's usage line for t, and shift+t too in the full list", () => {
    const plain = painter(false);

    expect(tvUsage(plain, false)).toBe('› Press t │ open on TV\n');
    expect(tvUsage(plain, true)).toBe(
      '› Press t │ open on TV\n› shift+t │ select a TV or a simulator\n',
    );
  });

  it('makes the key bold and the bar dim, as Expo does', () => {
    expect(tvUsage(painter(true), false)).toBe(
      '› Press \u001b[1mt\u001b[22m \u001b[2m│\u001b[22m open on TV\n',
    );
  });
});
