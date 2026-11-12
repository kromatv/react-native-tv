import { describe, expect, it } from 'vitest';
import { selectForTizen } from '../../react-native-overrides/Libraries/Utilities/select-for-tizen.ts';

describe('selectForTizen', () => {
  it('prefers the tizen entry over every other', () => {
    expect(selectForTizen({ tizen: 't', android: 'a', native: 'n', default: 'd' })).toBe('t');
  });

  it('falls back to android, since a Tizen bundle runs the Android sources', () => {
    expect(selectForTizen({ ios: 'i', android: 'a', native: 'n', default: 'd' })).toBe('a');
  });

  it('falls back to native, then default', () => {
    expect(selectForTizen({ ios: 'i', native: 'n', default: 'd' })).toBe('n');
    expect(selectForTizen({ ios: 'i', default: 'd' })).toBe('d');
  });

  it('never picks the ios entry', () => {
    expect(selectForTizen({ ios: 'i' })).toBeUndefined();
  });

  it('takes a present key even when its value is undefined, as React Native does', () => {
    expect(selectForTizen({ android: undefined, default: 'd' })).toBeUndefined();
  });
});
