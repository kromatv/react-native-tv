import { BlurView } from 'expo-blur';
import { Image, StyleSheet, Text, View } from 'react-native';
import poster from '../rendering/poster.png';
import { ScreenFrame, palette } from '../rendering/screen-frame.tsx';

const STRIPES = Array.from({ length: 12 }, (_, index) => index);

export function BlurScreen() {
  return (
    <ScreenFrame
      title="BlurView over a still backdrop"
      hint="1 still · 2 rail blurred · 3 rail tinted · 4 masks"
    >
      <View style={styles.stage}>
        <Image source={poster} resizeMode="cover" style={styles.poster} />
        <View style={styles.stripes}>
          {STRIPES.map((index) => (
            <View key={index} style={[styles.stripe, index % 2 === 0 && styles.stripeLight]} />
          ))}
        </View>
        <Text style={styles.word}>BACKDROP</Text>
        <BlurView tint="dark" intensity={64} style={styles.pill}>
          <Text style={styles.label}>dark 64, a capsule</Text>
        </BlurView>
        <BlurView tint="light" intensity={100} style={styles.square}>
          <Text style={[styles.label, styles.dark]}>light 100</Text>
        </BlurView>
        <BlurView tint="default" intensity={24} style={styles.thin}>
          <Text style={styles.label}>default 24</Text>
        </BlurView>
        <BlurView tint="dark" intensity={0} style={styles.none}>
          <Text style={styles.label}>dark 0</Text>
        </BlurView>
      </View>
    </ScreenFrame>
  );
}

const styles = StyleSheet.create({
  stage: { flex: 1, borderRadius: 24, overflow: 'hidden', backgroundColor: palette.card },
  stripes: { ...StyleSheet.absoluteFill, flexDirection: 'row' },
  poster: { ...StyleSheet.absoluteFill, width: '100%', height: '100%' },
  stripe: { flex: 1, backgroundColor: 'rgba(255, 64, 64, 0.35)' },
  stripeLight: { backgroundColor: 'rgba(255, 255, 255, 0.5)' },
  word: {
    position: 'absolute',
    top: 300,
    left: 80,
    color: palette.amber,
    fontSize: 160,
    fontWeight: '700',
  },
  pill: {
    position: 'absolute',
    top: 60,
    left: 520,
    width: 760,
    height: 96,
    borderRadius: 48,
    justifyContent: 'center',
    alignItems: 'center',
  },
  square: {
    position: 'absolute',
    top: 260,
    left: 160,
    width: 520,
    height: 340,
    justifyContent: 'center',
    alignItems: 'center',
  },
  thin: {
    position: 'absolute',
    top: 260,
    left: 760,
    width: 420,
    height: 340,
    borderRadius: 32,
    justifyContent: 'center',
    alignItems: 'center',
  },
  none: {
    position: 'absolute',
    top: 260,
    left: 1260,
    width: 340,
    height: 340,
    justifyContent: 'center',
    alignItems: 'center',
  },
  label: { color: palette.text, fontSize: 32, fontWeight: '700' },
  dark: { color: '#111111' },
});
