import { useRef, useState } from 'react';
import { ScrollView, StyleSheet, Text, View, useTVEventHandler } from 'react-native';
import { ScreenFrame, palette } from './screen-frame.tsx';

const CARD_WIDTH = 280;
const CARD_GAP = 24;
const CARDS = Array.from({ length: 16 }, (_, index) => index);
const ROW_HEIGHT = 64;
const ROWS = Array.from({ length: 40 }, (_, index) => index);

export function ScrollScreen() {
  const rail = useRef<ScrollView>(null);
  const list = useRef<ScrollView>(null);
  const [card, setCard] = useState(0);
  const [row, setRow] = useState(0);
  const [railX, setRailX] = useState(0);
  const [listY, setListY] = useState(0);
  const [settled, setSettled] = useState(0);
  useTVEventHandler((event) => {
    if (event.eventType === 'right' || event.eventType === 'left') {
      const next = Math.max(
        0,
        Math.min(CARDS.length - 1, card + (event.eventType === 'right' ? 1 : -1)),
      );
      setCard(next);
      rail.current?.scrollTo({
        x: Math.max(0, (next - 1) * (CARD_WIDTH + CARD_GAP)),
        animated: true,
      });
    }
    if (event.eventType === 'down' || event.eventType === 'up') {
      const next = Math.max(
        0,
        Math.min(ROWS.length - 1, row + (event.eventType === 'down' ? 1 : -1)),
      );
      setRow(next);
      list.current?.scrollTo({ y: Math.max(0, (next - 2) * ROW_HEIGHT), animated: false });
    }
    if (event.eventType === 'select') list.current?.scrollToEnd({ animated: true });
  });
  return (
    <ScreenFrame
      title="ScrollView"
      hint="Left/Right: rail (animated) · Up/Down: list · Enter: list scrollToEnd"
    >
      <Text
        style={styles.status}
      >{`rail x=${railX.toFixed(0)} · list y=${listY.toFixed(0)} · settled ${settled}`}</Text>
      <ScrollView
        ref={rail}
        horizontal
        scrollEventThrottle={16}
        onScroll={(event) => setRailX(event.nativeEvent.contentOffset.x)}
        onMomentumScrollEnd={() => setSettled((count) => count + 1)}
        style={styles.rail}
        contentContainerStyle={styles.railContent}
      >
        {CARDS.map((index) => (
          <View key={index} style={[styles.card, index === card && styles.cardFocused]}>
            <Text style={styles.cardLabel}>{`Card ${index + 1}`}</Text>
          </View>
        ))}
      </ScrollView>
      <ScrollView
        ref={list}
        scrollEventThrottle={16}
        onScroll={(event) => setListY(event.nativeEvent.contentOffset.y)}
        style={styles.list}
      >
        {ROWS.map((index) => (
          <View key={index} style={[styles.row, index === row && styles.rowFocused]}>
            <Text style={styles.rowLabel}>{`Row ${index + 1}`}</Text>
          </View>
        ))}
      </ScrollView>
    </ScreenFrame>
  );
}

const styles = StyleSheet.create({
  status: { color: palette.amber, fontSize: 26 },
  rail: { flexGrow: 0, marginTop: 24, borderWidth: 2, borderColor: '#33405e' },
  railContent: { gap: CARD_GAP, padding: 24 },
  card: {
    width: CARD_WIDTH,
    height: 180,
    backgroundColor: palette.card,
    borderRadius: 16,
    padding: 24,
  },
  cardFocused: { backgroundColor: palette.accent },
  cardLabel: { color: palette.text, fontSize: 32, fontWeight: '600' },
  list: { flex: 1, marginTop: 24, width: 720, borderWidth: 2, borderColor: '#33405e' },
  row: { height: ROW_HEIGHT, justifyContent: 'center', paddingHorizontal: 24 },
  rowFocused: { backgroundColor: palette.card },
  rowLabel: { color: palette.muted, fontSize: 28 },
});
