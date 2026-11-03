import { BlurView } from 'expo-blur';
import { useEffect, useRef } from 'react';
import { ScrollView, StyleSheet, Text, View } from 'react-native';
import { ScreenFrame, palette } from '../rendering/screen-frame.tsx';

const CARD_WIDTH = 320;
const CARD_GAP = 24;
const STEP = CARD_WIDTH + CARD_GAP;
const CARDS = Array.from({ length: 24 }, (_, index) => index);
const HUES = ['#e53935', '#8e24aa', '#3949ab', '#00897b', '#fdd835', '#fb8c00'] as const;
const SCROLL_EVERY_MS = 400;
const TRAVEL = 8;

function useAutoScroll() {
  const rail = useRef<ScrollView>(null);
  useEffect(() => {
    let position = 0;
    let direction = 1;
    const timer = setInterval(() => {
      if (position + direction < 0 || position + direction > TRAVEL) direction = -direction;
      position += direction;
      rail.current?.scrollTo({ x: position * STEP, animated: true });
    }, SCROLL_EVERY_MS);
    return () => clearInterval(timer);
  }, []);
  return rail;
}

/** A rail scrolling on its own under a header, or below it: blurred, or tinted only, to compare frame times. */
export function RailBlurScreen({
  blurred,
  under = true,
}: {
  readonly blurred: boolean;
  readonly under?: boolean;
}) {
  const rail = useAutoScroll();
  return (
    <ScreenFrame
      title={`Rail ${under ? 'under' : 'below'} a ${blurred ? 'BlurView' : 'tint'}`}
      hint="Scrolls by itself, one card every 400 ms"
    >
      <ScrollView
        ref={rail}
        horizontal
        style={[styles.rail, !under && styles.railBelow]}
        contentContainerStyle={styles.railContent}
      >
        {CARDS.map((index) => (
          <View
            key={index}
            style={[
              styles.card,
              {
                experimental_backgroundImage: `linear-gradient(160deg, ${HUES[index % HUES.length]}, #101010)`,
              },
            ]}
          >
            <Text style={styles.cardLabel}>{`Card ${index + 1}`}</Text>
          </View>
        ))}
      </ScrollView>
      {blurred ? (
        <BlurView tint="dark" intensity={64} style={styles.header}>
          <Text style={styles.headerLabel}>Header</Text>
        </BlurView>
      ) : (
        <View style={[styles.header, styles.tint]}>
          <Text style={styles.headerLabel}>Header</Text>
        </View>
      )}
    </ScreenFrame>
  );
}

const styles = StyleSheet.create({
  rail: { flexGrow: 0, marginTop: 40 },
  railBelow: { marginTop: 240 },
  railContent: { gap: CARD_GAP, paddingHorizontal: 24 },
  card: {
    width: CARD_WIDTH,
    height: 460,
    borderRadius: 20,
    justifyContent: 'flex-end',
    padding: 24,
  },
  cardLabel: { color: palette.text, fontSize: 40, fontWeight: '700' },
  header: {
    position: 'absolute',
    top: 0,
    left: 0,
    right: 0,
    height: 200,
    borderRadius: 32,
    justifyContent: 'center',
    paddingHorizontal: 48,
  },
  tint: { backgroundColor: 'rgba(25, 25, 25, 0.44)' },
  headerLabel: { color: palette.text, fontSize: 48, fontWeight: '700' },
});
