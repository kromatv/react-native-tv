import { colorAttribute } from 'react-native/Libraries/Components/View/ReactNativeStyleAttributes';
import * as NativeComponentRegistry from 'react-native/Libraries/NativeComponent/NativeComponentRegistry';
import codegenNativeCommands from 'react-native/Libraries/Utilities/codegenNativeCommands';

/** The `RCTImageView` commands, as in React Native. */
export const Commands = codegenNativeCommands({
  supportedCommands: ['setIsVisible_EXPERIMENTAL'],
});

/**
 * React Native's Android view config for `RCTImageView`, which
 * `ImageViewNativeComponent.js` gives only when `Platform.OS` is `android`.
 * Tizen runs `Image.android.js`, which sends `src`, `headers`, a `defaultSource`
 * URI and `shouldNotifyLoadEvents`; the iOS config drops all four.
 */
export const __INTERNAL_VIEW_CONFIG = {
  uiViewClassName: 'RCTImageView',
  bubblingEventTypes: {},
  directEventTypes: {
    topLoadStart: { registrationName: 'onLoadStart' },
    topProgress: { registrationName: 'onProgress' },
    topError: { registrationName: 'onError' },
    topLoad: { registrationName: 'onLoad' },
    topLoadEnd: { registrationName: 'onLoadEnd' },
  },
  validAttributes: {
    blurRadius: true,
    defaultSource: true,
    internal_analyticTag: true,
    resizeMethod: true,
    resizeMode: true,
    resizeMultiplier: true,
    tintColor: colorAttribute,
    borderBottomLeftRadius: true,
    borderTopLeftRadius: true,
    src: true,
    source: true,
    borderRadius: true,
    headers: true,
    shouldNotifyLoadEvents: true,
    overlayColor: colorAttribute,
    borderColor: colorAttribute,
    accessible: true,
    progressiveRenderingEnabled: true,
    fadeDuration: true,
    borderBottomRightRadius: true,
    borderTopRightRadius: true,
    loadingIndicatorSrc: true,
  },
};

const ImageViewNativeComponent = NativeComponentRegistry.get(
  'RCTImageView',
  () => __INTERNAL_VIEW_CONFIG,
);

export default ImageViewNativeComponent;
