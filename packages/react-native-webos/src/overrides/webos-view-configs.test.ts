import { describe, expect, it } from 'vitest';
import { webosViewConfig } from '../../react-native-overrides/Libraries/ReactNative/webos-view-configs.ts';

describe('webosViewConfig', () => {
  it('gives the masked view a config under its own name with no props of its own', () => {
    expect(webosViewConfig('RNCMaskedView')).toEqual({
      uiViewClassName: 'RNCMaskedView',
      bubblingEventTypes: {},
      directEventTypes: {},
      validAttributes: {},
    });
  });

  it('knows nothing of other legacy views or of inherited object keys', () => {
    expect(webosViewConfig('RNCWebView')).toBeNull();
    expect(webosViewConfig('toString')).toBeNull();
  });
});
