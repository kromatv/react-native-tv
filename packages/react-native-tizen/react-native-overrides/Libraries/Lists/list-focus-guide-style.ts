const FLEX_KEYS = ['flex', 'flexGrow', 'flexShrink', 'flexBasis', 'alignSelf'] as const;

type FlexKey = (typeof FLEX_KEYS)[number];

/** The props that decide how a box takes space from its parent. */
export type FlexStyle = Partial<Record<FlexKey, unknown>>;

const SCROLL_VIEW_FLEX: FlexStyle = { flexGrow: 1, flexShrink: 1 };

function isFlexKey(key: string): key is FlexKey {
  return (FLEX_KEYS as readonly string[]).includes(key);
}

function collect(style: unknown, into: FlexStyle): void {
  if (Array.isArray(style)) {
    for (const entry of style) collect(entry, into);
    return;
  }
  if (typeof style !== 'object' || style === null) return;
  const entries: [string, unknown][] = Object.entries(style);
  for (const [key, value] of entries) {
    if (isFlexKey(key)) into[key] = value;
  }
}

/**
 * The style of the `TVFocusGuideView` react-native-tvos wraps a list in: the
 * flex its ScrollView would take from the list's parent, React Native's own
 * `flexGrow: 1, flexShrink: 1` overridden by the list's `style`, as a
 * flattened style would be. The ScrollView then fills the wrapper.
 */
export function listFocusGuideStyle(listStyle: unknown): FlexStyle {
  const flex: FlexStyle = { ...SCROLL_VIEW_FLEX };
  collect(listStyle, flex);
  return flex;
}
