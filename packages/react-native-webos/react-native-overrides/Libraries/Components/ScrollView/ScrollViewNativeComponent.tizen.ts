import { colorAttribute } from 'react-native/Libraries/Components/View/ReactNativeStyleAttributes';
import * as NativeComponentRegistry from 'react-native/Libraries/NativeComponent/NativeComponentRegistry';
import pointsDiffer from 'react-native/Libraries/Utilities/differ/pointsDiffer';

/**
 * React Native's Android view config for `RCTScrollView`, which
 * `ScrollViewNativeComponent.js` gives only when `Platform.OS` is `android`.
 * Tizen runs the Android ScrollView, whose props (`horizontal`,
 * `scrollsChildToFocus`, `isInvertedVirtualizedList`...) the iOS config drops.
 */
export const __INTERNAL_VIEW_CONFIG = {
  uiViewClassName: 'RCTScrollView',
  bubblingEventTypes: {},
  directEventTypes: {
    topMomentumScrollBegin: { registrationName: 'onMomentumScrollBegin' },
    topMomentumScrollEnd: { registrationName: 'onMomentumScrollEnd' },
    topScroll: { registrationName: 'onScroll' },
    topScrollBeginDrag: { registrationName: 'onScrollBeginDrag' },
    topScrollEndDrag: { registrationName: 'onScrollEndDrag' },
  },
  validAttributes: {
    contentOffset: { diff: pointsDiffer },
    decelerationRate: true,
    disableIntervalMomentum: true,
    maintainVisibleContentPosition: true,
    pagingEnabled: true,
    scrollEnabled: true,
    showsVerticalScrollIndicator: true,
    snapToAlignment: true,
    snapToEnd: true,
    snapToInterval: true,
    snapToOffsets: true,
    snapToStart: true,
    borderBottomLeftRadius: true,
    borderBottomRightRadius: true,
    sendMomentumEvents: true,
    borderRadius: true,
    nestedScrollEnabled: true,
    scrollEventThrottle: true,
    borderStyle: true,
    borderRightColor: colorAttribute,
    borderColor: colorAttribute,
    borderBottomColor: colorAttribute,
    persistentScrollbar: true,
    horizontal: true,
    endFillColor: colorAttribute,
    fadingEdgeLength: true,
    overScrollMode: true,
    borderTopLeftRadius: true,
    scrollPerfTag: true,
    borderTopColor: colorAttribute,
    removeClippedSubviews: true,
    borderTopRightRadius: true,
    borderLeftColor: colorAttribute,
    snapToItemPadding: true,
    scrollAnimationEnabled: true,
    scrollAnimationDuration: true,
    scrollAnimationEasing: true,
    pointerEvents: true,
    isInvertedVirtualizedList: true,
    scrollsChildToFocus: true,
  },
};

const ScrollViewNativeComponent = NativeComponentRegistry.get(
  'RCTScrollView',
  () => __INTERNAL_VIEW_CONFIG,
);

export default ScrollViewNativeComponent;
