import { useEffect, useRef } from 'react';
import { Image, ScrollView, StyleSheet, Text, View } from 'react-native';
import Svg, { Defs, LinearGradient, Rect, Stop } from 'react-native-svg';
import poster from '../rendering/poster.png';
import { ScreenFrame, palette } from '../rendering/screen-frame.tsx';
import { NativeMaskedView } from './native-masked-view.tsx';

const RAMP = 120;
const CARDS = Array.from({ length: 16 }, (_, index) => index);
const HUES = ['#e53935', '#8e24aa', '#3949ab', '#00897b', '#fdd835', '#fb8c00'] as const;

function EdgeFade() {
  return (
    <Svg width="100%" height="100%">
      <Defs>
        <LinearGradient id="edge-fade" x1="0" y1="0" x2="1" y2="0">
          <Stop offset={0} stopColor="#000" stopOpacity={0} />
          <Stop offset={0.12} stopColor="#000" stopOpacity={1} />
          <Stop offset={0.88} stopColor="#000" stopOpacity={1} />
          <Stop offset={1} stopColor="#000" stopOpacity={0} />
        </LinearGradient>
      </Defs>
      <Rect x="0" y="0" width="100%" height="100%" fill="url(#edge-fade)" />
    </Svg>
  );
}

function TopRamp() {
  return (
    <View style={styles.fill}>
      <Svg width="100%" height={RAMP}>
        <Defs>
          <LinearGradient id="top-ramp" x1="0" y1="0" x2="0" y2="1">
            <Stop offset={0} stopColor="#000" stopOpacity={0} />
            <Stop offset={1} stopColor="#000" stopOpacity={1} />
          </LinearGradient>
        </Defs>
        <Rect x="0" y="0" width="100%" height={RAMP} fill="url(#top-ramp)" />
      </Svg>
      <View style={styles.solid} />
    </View>
  );
}

function useSweep() {
  const rail = useRef<ScrollView>(null);
  useEffect(() => {
    let far = false;
    const timer = setInterval(() => {
      far = !far;
      rail.current?.scrollTo({ x: far ? 1200 : 0, animated: true });
    }, 1500);
    return () => clearInterval(timer);
  }, []);
  return rail;
}

export function MaskScreen() {
  const rail = useSweep();
  return (
    <ScreenFrame
      title="MaskedView"
      hint="Text mask · Svg edge fade on a moving rail · CSS gradient mask · ramp"
    >
      <View style={styles.row}>
        <NativeMaskedView
          style={styles.word}
          maskElement={<Text style={styles.maskText}>KROMA</Text>}
        >
          <View style={styles.rainbow} />
        </NativeMaskedView>
        <NativeMaskedView style={styles.photo} maskElement={<View style={styles.cssMask} />}>
          <Image source={poster} resizeMode="cover" style={styles.poster} />
        </NativeMaskedView>
        <NativeMaskedView style={styles.photo} maskElement={<TopRamp />}>
          <Image source={poster} resizeMode="cover" style={styles.poster} />
        </NativeMaskedView>
      </View>
      <View style={styles.railStage}>
        <NativeMaskedView style={styles.railMask} maskElement={<EdgeFade />}>
          <ScrollView ref={rail} horizontal contentContainerStyle={styles.railContent}>
            {CARDS.map((index) => (
              <View
                key={index}
                style={[styles.card, { backgroundColor: HUES[index % HUES.length] }]}
              >
                <Text style={styles.cardLabel}>{index + 1}</Text>
              </View>
            ))}
          </ScrollView>
        </NativeMaskedView>
      </View>
    </ScreenFrame>
  );
}

const styles = StyleSheet.create({
  row: { flexDirection: 'row', gap: 48, height: 340 },
  word: { width: 720, height: 220 },
  maskText: { fontSize: 180, fontWeight: '700', color: '#000' },
  rainbow: {
    flex: 1,
    experimental_backgroundImage:
      'linear-gradient(90deg, #e53935, #fdd835, #00897b, #3949ab, #8e24aa)',
  },
  photo: { width: 440, height: 340, borderRadius: 24 },
  cssMask: {
    flex: 1,
    borderRadius: 120,
    experimental_backgroundImage: 'radial-gradient(circle, #000 50%, transparent 72%)',
  },
  fill: { flex: 1 },
  poster: { width: '100%', height: '100%' },
  solid: { flex: 1, backgroundColor: '#000' },
  railStage: { marginTop: 56, height: 260, backgroundColor: palette.card },
  railMask: { flex: 1 },
  railContent: { gap: 24, padding: 24 },
  card: {
    width: 300,
    height: 212,
    borderRadius: 16,
    justifyContent: 'center',
    alignItems: 'center',
  },
  cardLabel: { color: palette.text, fontSize: 64, fontWeight: '700' },
});
