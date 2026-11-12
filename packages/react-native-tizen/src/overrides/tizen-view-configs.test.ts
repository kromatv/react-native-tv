import { describe, expect, it } from 'vitest';
import { tizenViewConfig } from '../../react-native-overrides/Libraries/ReactNative/tizen-view-configs.ts';

describe('tizenViewConfig', () => {
  it('gives the masked view a config under its own name with no props of its own', () => {
    expect(tizenViewConfig('RNCMaskedView')).toEqual({
      uiViewClassName: 'RNCMaskedView',
      bubblingEventTypes: {},
      directEventTypes: {},
      validAttributes: {},
    });
  });

  it('knows nothing of other legacy views or of inherited object keys', () => {
    expect(tizenViewConfig('RNCWebView')).toBeNull();
    expect(tizenViewConfig('toString')).toBeNull();
  });
});
