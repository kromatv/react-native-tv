import { StyleSheet, Text, View, type ViewStyle } from 'react-native';
import { ScreenFrame, palette } from './screen-frame.tsx';

interface Sample {
  readonly label: string;
  readonly style: ViewStyle;
  readonly overflowChild?: boolean;
}

const SAMPLES: readonly Sample[] = [
  { label: 'radius 32', style: { borderRadius: 32 } },
  {
    label: 'per-corner radii',
    style: { borderTopLeftRadius: 64, borderBottomRightRadius: 64, borderTopRightRadius: 8 },
  },
  {
    label: 'border 6, radius 24',
    style: { borderRadius: 24, borderWidth: 6, borderColor: palette.amber },
  },
  {
    label: 'per-side borders',
    style: {
      borderTopWidth: 4,
      borderTopColor: '#ff5252',
      borderRightWidth: 10,
      borderRightColor: '#69f0ae',
      borderBottomWidth: 16,
      borderBottomColor: '#40c4ff',
      borderLeftWidth: 2,
      borderLeftColor: '#ffd740',
    },
  },
  {
    label: 'overflow hidden + radius',
    style: { borderRadius: 48, overflow: 'hidden' },
    overflowChild: true,
  },
  { label: 'overflow visible', style: { borderRadius: 48 }, overflowChild: true },
  {
    label: 'boxShadow',
    style: { borderRadius: 24, boxShadow: '0px 16px 32px rgba(0, 0, 0, 0.9)' },
  },
  {
    label: 'amber glow + spread',
    style: { borderRadius: 24, boxShadow: '0px 0px 24px 6px rgba(255, 179, 0, 0.8)' },
  },
  {
    label: 'outline offset 6',
    style: {
      borderRadius: 24,
      outlineWidth: 4,
      outlineColor: palette.amber,
      outlineOffset: 6,
      outlineStyle: 'solid',
    },
  },
  {
    label: 'iOS shadow props',
    style: {
      borderRadius: 16,
      shadowColor: '#000',
      shadowOpacity: 0.9,
      shadowRadius: 12,
      shadowOffset: { width: 12, height: 12 },
    },
  },
  { label: 'opacity 0.5', style: { borderRadius: 24, opacity: 0.5 } },
  {
    label: 'transparent, border only',
    style: {
      backgroundColor: 'transparent',
      borderRadius: 999,
      borderWidth: 4,
      borderColor: '#ff80ab',
    },
  },
];

export function BoxScreen() {
  return (
    <ScreenFrame
      title="Boxes"
      hint="Rounded corners, borders, overflow clipping, shadows and outlines (nine-patches)"
    >
      <View style={styles.grid}>
        {SAMPLES.map((sample) => (
          <View key={sample.label} style={styles.cell}>
            <View style={[styles.box, sample.style]}>
              {sample.overflowChild ? <View style={styles.overflowChild} /> : null}
            </View>
            <Text style={styles.label}>{sample.label}</Text>
          </View>
        ))}
      </View>
    </ScreenFrame>
  );
}

const styles = StyleSheet.create({
  grid: { flexDirection: 'row', flexWrap: 'wrap', columnGap: 56, rowGap: 40, paddingTop: 16 },
  cell: { width: 360, alignItems: 'center' },
  box: { width: 320, height: 180, backgroundColor: palette.accent },
  overflowChild: {
    position: 'absolute',
    left: -40,
    top: 60,
    width: 200,
    height: 160,
    backgroundColor: palette.amber,
  },
  label: { color: palette.muted, fontSize: 24, marginTop: 12 },
});
