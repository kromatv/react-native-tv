import { describe, expect, it } from 'vitest';
import { painter } from '../paint.ts';
import { webosUsage } from './webos-usage.ts';

describe('webosUsage', () => {
  it("prints Expo's usage line for t, and shift+t too in the full list", () => {
    const plain = painter(false);

    expect(webosUsage(plain, false)).toBe('› Press t │ open on webOS TV\n');
    expect(webosUsage(plain, true)).toBe(
      '› Press t │ open on webOS TV\n› shift+t │ select a webOS TV or the simulator\n',
    );
  });

  it('makes the key bold and the bar dim, as Expo does', () => {
    expect(webosUsage(painter(true), false)).toBe(
      '› Press \u001b[1mt\u001b[22m \u001b[2m│\u001b[22m open on webOS TV\n',
    );
  });
});
