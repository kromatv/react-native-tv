/** What setting the `react-native` override did to the file a package manager reads it from. */
export type OverrideSetup =
  | { readonly kind: 'set' | 'present'; readonly file: string }
  | { readonly kind: 'manual'; readonly file: string; readonly lines: readonly string[] };
