import { describe, expect, it } from 'vitest';
import { selectForWebOS } from '../../react-native-overrides/Libraries/Utilities/select-for-webos.ts';

describe('selectForWebOS', () => {
  it('prefers the webos entry over every other', () => {
    expect(selectForWebOS({ webos: 't', android: 'a', native: 'n', default: 'd' })).toBe('t');
  });

  it('falls back to android, since a webOS bundle runs the Android sources', () => {
    expect(selectForWebOS({ ios: 'i', android: 'a', native: 'n', default: 'd' })).toBe('a');
  });

  it('falls back to native, then default', () => {
    expect(selectForWebOS({ ios: 'i', native: 'n', default: 'd' })).toBe('n');
    expect(selectForWebOS({ ios: 'i', default: 'd' })).toBe('d');
  });

  it('never picks the ios entry', () => {
    expect(selectForWebOS({ ios: 'i' })).toBeUndefined();
  });

  it('takes a present key even when its value is undefined, as React Native does', () => {
    expect(selectForWebOS({ android: undefined, default: 'd' })).toBeUndefined();
  });
});
