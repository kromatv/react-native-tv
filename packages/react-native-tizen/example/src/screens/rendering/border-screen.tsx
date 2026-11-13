import { useEffect, useState } from 'react';
import { Animated, Easing, StyleSheet, Text, View, type ViewStyle } from 'react-native';
import { ScreenFrame, palette } from './screen-frame.tsx';

const FOUR_COLORS: ViewStyle = {
  borderTopColor: '#ff5252',
  borderRightColor: '#69f0ae',
  borderBottomColor: '#40c4ff',
  borderLeftColor: '#ffd740',
};

const TOP_AND_RIGHT: ViewStyle = {
  borderTopColor: palette.amber,
  borderRightColor: palette.amber,
  borderBottomColor: 'transparent',
  borderLeftColor: 'transparent',
};

const SAMPLES: readonly { readonly label: string; readonly style: ViewStyle }[] = [
  {
    label: 'two sides of a circle',
    style: { width: 140, borderRadius: 9999, borderWidth: 14, ...TOP_AND_RIGHT },
  },
  {
    label: 'four colors, circle',
    style: { width: 140, borderRadius: 9999, borderWidth: 14, ...FOUR_COLORS },
  },
  { label: 'four colors, radius 32', style: { borderRadius: 32, borderWidth: 14, ...FOUR_COLORS } },
  {
    label: 'widths 28, 6, 14, 6',
    style: {
      borderRadius: 40,
      borderTopWidth: 28,
      borderRightWidth: 6,
      borderBottomWidth: 14,
      borderLeftWidth: 6,
      ...FOUR_COLORS,
    },
  },
  { label: 'ellipse', style: { borderRadius: '50%', borderWidth: 14, ...FOUR_COLORS } },
  {
    label: 'one square corner',
    style: { borderRadius: 40, borderTopLeftRadius: 0, borderWidth: 14, ...FOUR_COLORS },
  },
  {
    label: 'two translucent sides',
    style: {
      borderRadius: 40,
      borderWidth: 14,
      ...FOUR_COLORS,
      borderTopColor: 'rgba(255, 82, 82, 0.4)',
      borderBottomColor: 'rgba(64, 196, 255, 0.4)',
    },
  },
  {
    label: 'dashed, four colors',
    style: { borderRadius: 40, borderWidth: 8, borderStyle: 'dashed', ...FOUR_COLORS },
  },
  {
    label: 'dotted, two sides',
    style: { borderRadius: 40, borderWidth: 8, borderStyle: 'dotted', ...TOP_AND_RIGHT },
  },
  {
    label: 'one color',
    style: { borderRadius: 40, borderWidth: 14, borderColor: palette.amber },
  },
];

const SPINNERS: readonly { readonly size: number; readonly stroke: number }[] = [
  { size: 28, stroke: 3 },
  { size: 56, stroke: 6 },
  { size: 112, stroke: 12 },
];

type Turn = Animated.AnimatedInterpolation<string> | string;

function useTurn(): Animated.AnimatedInterpolation<string> {
  const [progress] = useState(() => new Animated.Value(0));
  useEffect(() => {
    const loop = Animated.loop(
      Animated.timing(progress, {
        toValue: 1,
        duration: 1200,
        easing: Easing.linear,
        useNativeDriver: true,
      }),
    );
    loop.start();
    return () => loop.stop();
  }, [progress]);
  return progress.interpolate({ inputRange: [0, 1], outputRange: ['0deg', '360deg'] });
}

interface SpinnerProps {
  readonly size: number;
  readonly stroke: number;
  readonly turn: Turn;
}

function Spinner({ size, stroke, turn }: SpinnerProps) {
  return (
    <Animated.View style={{ width: size, height: size, transform: [{ rotate: turn }] }}>
      <View style={[styles.ring, styles.track, { borderWidth: stroke }]} />
      <View style={[styles.ring, TOP_AND_RIGHT, { borderWidth: stroke }]} />
    </Animated.View>
  );
}

export function BorderScreen() {
  const turn = useTurn();
  return (
    <ScreenFrame
      title="Border sides"
      hint="A color per side on rounded corners, and a spinner made of two rings"
    >
      <View style={styles.grid}>
        {SAMPLES.map((sample) => (
          <View key={sample.label} style={styles.cell}>
            <View style={[styles.box, sample.style]} />
            <Text style={styles.label}>{sample.label}</Text>
          </View>
        ))}
      </View>
      <View style={styles.spinners}>
        {SPINNERS.map((spinner) => (
          <Spinner key={spinner.size} turn={turn} {...spinner} />
        ))}
        <Text style={styles.label}>turning, then still at 0deg and 45deg</Text>
        <Spinner size={112} stroke={12} turn="0deg" />
        <Spinner size={112} stroke={12} turn="45deg" />
      </View>
    </ScreenFrame>
  );
}

const styles = StyleSheet.create({
  grid: { flexDirection: 'row', flexWrap: 'wrap', columnGap: 40, rowGap: 32, paddingTop: 8 },
  cell: { width: 300, alignItems: 'center' },
  box: { width: 240, height: 140 },
  label: { color: palette.muted, fontSize: 24, marginTop: 12 },
  spinners: { flexDirection: 'row', alignItems: 'center', gap: 48, marginTop: 56 },
  ring: { position: 'absolute', inset: 0, borderRadius: 9999 },
  track: { opacity: 0.3, borderColor: palette.amber },
});
