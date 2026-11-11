import { useState } from 'react';
import { StyleSheet, Text, View, useTVEventHandler, type ViewStyle } from 'react-native';
import { ScreenFrame, palette } from './screen-frame.tsx';

const CARDS = Array.from({ length: 5 }, (_, index) => index);

const SAMPLES: readonly { readonly label: string; readonly style: ViewStyle }[] = [
  { label: 'translateX 40', style: { transform: [{ translateX: 40 }] } },
  { label: 'rotate 15deg', style: { transform: [{ rotate: '15deg' }] } },
  {
    label: 'scale 0.7, origin top left',
    style: { transform: [{ scale: 0.7 }], transformOrigin: 'top left' },
  },
  {
    label: 'perspective + rotateY',
    style: { transform: [{ perspective: 600 }, { rotateY: '35deg' }] },
  },
];

export function TransformScreen() {
  const [focused, setFocused] = useState(0);
  useTVEventHandler((event) => {
    if (event.eventType === 'right') setFocused((index) => Math.min(CARDS.length - 1, index + 1));
    if (event.eventType === 'left') setFocused((index) => Math.max(0, index - 1));
  });
  return (
    <ScreenFrame
      title="Transforms"
      hint="Left/Right: focus scale 1.12 around the center, with shadow and outline"
    >
      <View style={styles.row}>
        {CARDS.map((index) => (
          <View key={index} style={[styles.card, index === focused && styles.focused]}>
            <Text style={styles.cardLabel}>{`Poster ${index + 1}`}</Text>
            <View style={styles.badge} />
          </View>
        ))}
      </View>
      <View style={styles.row}>
        {SAMPLES.map((sample) => (
          <View key={sample.label} style={styles.cell}>
            <View style={[styles.sample, sample.style]}>
              <Text style={styles.cardLabel}>Aa</Text>
            </View>
            <Text style={styles.label}>{sample.label}</Text>
          </View>
        ))}
      </View>
    </ScreenFrame>
  );
}

const styles = StyleSheet.create({
  row: { flexDirection: 'row', gap: 48, paddingVertical: 40, paddingHorizontal: 24 },
  card: {
    width: 240,
    height: 340,
    backgroundColor: palette.card,
    borderRadius: 20,
    padding: 24,
    justifyContent: 'flex-end',
  },
  focused: {
    backgroundColor: palette.accent,
    transform: [{ scale: 1.12 }],
    boxShadow: '0px 18px 36px rgba(0, 0, 0, 0.85)',
    outlineWidth: 4,
    outlineColor: palette.amber,
    outlineOffset: 6,
    outlineStyle: 'solid',
  },
  cardLabel: { color: palette.text, fontSize: 30, fontWeight: '600' },
  badge: {
    position: 'absolute',
    top: 16,
    right: 16,
    width: 40,
    height: 40,
    borderRadius: 20,
    backgroundColor: palette.amber,
  },
  cell: { width: 300, alignItems: 'center' },
  sample: {
    width: 200,
    height: 140,
    backgroundColor: '#00897b',
    borderRadius: 12,
    alignItems: 'center',
    justifyContent: 'center',
  },
  label: { color: palette.muted, fontSize: 24, marginTop: 24 },
});
