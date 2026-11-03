import { requireOptionalNativeModule } from 'expo';
import { PixelRatio, StyleSheet, Text, View, useWindowDimensions } from 'react-native';
import { Fact } from './event-log.tsx';

interface ExpoTizenModule {
  getScreenSize(): { readonly width: number; readonly height: number };
}

const expoTizen = requireOptionalNativeModule<ExpoTizenModule>('ExpoTizen');

const SIZES = [12, 16, 20, 28] as const;

export function ScreenSizeScreen() {
  const window = useWindowDimensions();
  const screen = expoTizen?.getScreenSize();
  return (
    <View>
      <Fact label="Window, in points" value={`${window.width}x${window.height}`} />
      <Fact label="PixelRatio" value={String(PixelRatio.get())} />
      <Fact
        label="Window, in pixels"
        value={screen ? `${screen.width}x${screen.height}` : 'no ExpoTizen module'}
      />
      <View style={styles.lines}>
        {[1, 0.5, StyleSheet.hairlineWidth].map((width) => (
          <View key={width} style={[styles.line, { height: width }]} />
        ))}
      </View>
      {SIZES.map((size) => (
        <Text key={size} style={[styles.sample, { fontSize: size }]}>
          {`${size} pt: The quick brown fox jumps over the lazy dog. 0123456789`}
        </Text>
      ))}
    </View>
  );
}

const styles = StyleSheet.create({
  lines: { gap: 12, marginVertical: 24 },
  line: { width: 900, backgroundColor: '#ffffff' },
  sample: { color: '#ffffff', marginBottom: 8 },
});
