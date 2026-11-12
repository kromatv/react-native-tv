import { describe, expect, it } from 'vitest';
import { painter } from '../paint.ts';
import { tizenUsage } from './tizen-usage.ts';

describe('tizenUsage', () => {
  it("prints Expo's usage line for t, and shift+t too in the full list", () => {
    const plain = painter(false);

    expect(tizenUsage(plain, false)).toBe('› Press t │ open on Tizen TV\n');
    expect(tizenUsage(plain, true)).toBe(
      '› Press t │ open on Tizen TV\n› shift+t │ select a Tizen TV or the simulator\n',
    );
  });

  it('makes the key bold and the bar dim, as Expo does', () => {
    expect(tizenUsage(painter(true), false)).toBe(
      '› Press \u001b[1mt\u001b[22m \u001b[2m│\u001b[22m open on Tizen TV\n',
    );
  });
});
