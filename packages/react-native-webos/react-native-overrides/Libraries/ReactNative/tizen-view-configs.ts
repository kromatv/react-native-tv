/** A view config as `NativeComponentRegistry.get` takes it. */
export interface PartialViewConfig {
  readonly uiViewClassName: string;
  readonly bubblingEventTypes: Readonly<Record<string, unknown>>;
  readonly directEventTypes: Readonly<Record<string, unknown>>;
  readonly validAttributes: Readonly<Record<string, unknown>>;
}

const VIEW_CONFIGS: Readonly<Record<string, PartialViewConfig>> = {
  RNCMaskedView: {
    uiViewClassName: 'RNCMaskedView',
    bubblingEventTypes: {},
    directEventTypes: {},
    validAttributes: {},
  },
};

/**
 * The view config of a native view the Tizen host draws with no codegen spec,
 * such as @react-native-masked-view/masked-view's `RNCMaskedView`, or null.
 * The base view's props are added by `NativeComponentRegistry.get`.
 */
export function tizenViewConfig(uiViewClassName: string): PartialViewConfig | null {
  return Object.hasOwn(VIEW_CONFIGS, uiViewClassName)
    ? (VIEW_CONFIGS[uiViewClassName] ?? null)
    : null;
}
