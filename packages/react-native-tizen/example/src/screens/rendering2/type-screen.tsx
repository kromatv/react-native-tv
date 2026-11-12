import { useFonts } from 'expo-font';
import { useState } from 'react';
import { type LayoutChangeEvent, StyleSheet, Text, View } from 'react-native';
import { ScreenFrame, palette } from '../rendering/screen-frame.tsx';

const FAMILY = 'Hanken Grotesk';
const FONTS = {
  [FAMILY]:
    'https://raw.githubusercontent.com/google/fonts/main/ofl/hankengrotesk/HankenGrotesk%5Bwght%5D.ttf',
};
const WEIGHTS = ['100', '300', '400', '500', '600', '700', '800', '900'] as const;
const SAMPLE = 'Hamburgefonstiv 0123';

function Measured({ style, children }: { readonly style: object; readonly children: string }) {
  const [width, setWidth] = useState(0);
  const onLayout = (event: LayoutChangeEvent) => setWidth(event.nativeEvent.layout.width);
  return (
    <View style={styles.row}>
      <Text onLayout={onLayout} style={[styles.sample, style]}>
        {children}
      </Text>
      <Text style={styles.width}>{width.toFixed(1)}</Text>
    </View>
  );
}

/** Hanken Grotesk, a variable font, at each weight and with letter spacing, each line with its measured width. */
export function TypeScreen() {
  const [loaded, error] = useFonts(FONTS);
  if (!loaded) {
    return (
      <ScreenFrame title="Variable font" hint={error ? error.message : 'Loading Hanken Grotesk…'}>
        <View />
      </ScreenFrame>
    );
  }
  return (
    <ScreenFrame
      title="Variable font, letter spacing"
      hint="Hanken Grotesk 32 px; widths in points"
    >
      <View style={styles.columns}>
        <View style={styles.column}>
          {WEIGHTS.map((weight) => (
            <Measured key={weight} style={{ fontWeight: weight }}>{`${weight} ${SAMPLE}`}</Measured>
          ))}
        </View>
        <View style={styles.column}>
          <Measured style={{ fontWeight: '700' }}>{SAMPLE}</Measured>
          <Measured style={{ fontWeight: '700', letterSpacing: -0.64 }}>{SAMPLE}</Measured>
          <Measured style={{ fontWeight: '700', letterSpacing: 3.84 }}>{SAMPLE}</Measured>
          <Measured style={styles.overline}>EN VEDETTE</Measured>
          <Text style={styles.wrapped}>
            Letter spacing that wraps: a paragraph set with four points between letters keeps its
            words whole.
          </Text>
          <Text numberOfLines={1} style={styles.clamped}>
            One clamped line with spacing ends in an ellipsis
          </Text>
        </View>
      </View>
    </ScreenFrame>
  );
}

const styles = StyleSheet.create({
  columns: { flexDirection: 'row', gap: 64 },
  column: { gap: 14, flex: 1 },
  row: { flexDirection: 'row', alignItems: 'baseline', gap: 16 },
  sample: { fontFamily: FAMILY, fontSize: 32, color: palette.text },
  width: { color: palette.muted, fontSize: 20 },
  overline: { fontWeight: '700', fontSize: 22, letterSpacing: 4.84, color: palette.amber },
  wrapped: {
    fontFamily: FAMILY,
    fontSize: 28,
    letterSpacing: 4,
    color: palette.text,
    width: 640,
    backgroundColor: palette.card,
  },
  clamped: {
    fontFamily: FAMILY,
    fontSize: 28,
    letterSpacing: 4,
    color: palette.text,
    width: 640,
    backgroundColor: palette.card,
  },
});
