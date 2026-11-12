import * as NativeComponentRegistry from 'react-native/Libraries/NativeComponent/NativeComponentRegistry';
import getNativeComponentAttributes from 'react-native/Libraries/ReactNative/getNativeComponentAttributes';
import createReactNativeComponentClass from 'react-native/Libraries/Renderer/shims/createReactNativeComponentClass';
import { tizenViewConfig } from './tizen-view-configs.ts';

/**
 * React Native's `requireNativeComponent`, which asks the native UI manager
 * for a legacy view's config. Tizen's host has no legacy interop layer, so a
 * view it draws without a codegen spec takes its config from
 * `tizen-view-configs.ts`; any other name behaves as in React Native.
 */
export default function requireNativeComponent(uiViewClassName: string): unknown {
  const config = tizenViewConfig(uiViewClassName);
  if (config !== null) return NativeComponentRegistry.get(uiViewClassName, () => config);
  return createReactNativeComponentClass(uiViewClassName, () =>
    getNativeComponentAttributes(uiViewClassName),
  );
}
