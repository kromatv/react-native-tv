import type { ReactNode } from 'react';
import {
  type HostComponent,
  StyleSheet,
  View,
  type ViewProps,
  requireNativeComponent,
} from 'react-native';

const RNCMaskedView: HostComponent<ViewProps> = requireNativeComponent<ViewProps>('RNCMaskedView');

/** What @react-native-masked-view/masked-view renders, without the dependency: the mask in an absolutely filled first child, the content after it. */
export function NativeMaskedView({
  maskElement,
  style,
  children,
}: {
  readonly maskElement: ReactNode;
  readonly style?: ViewProps['style'];
  readonly children: ReactNode;
}) {
  return (
    <RNCMaskedView style={style}>
      <View pointerEvents="none" style={StyleSheet.absoluteFill}>
        {maskElement}
      </View>
      {children}
    </RNCMaskedView>
  );
}
