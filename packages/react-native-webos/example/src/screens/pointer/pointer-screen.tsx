import { useState } from 'react';
import { Pressable, ScrollView, StyleSheet, Text, View } from 'react-native';

const TILES = ['Movies', 'Series', 'Live', 'Music', 'Settings'] as const;
const ROWS = Array.from({ length: 30 }, (_, index) => `Row ${index + 1}`);

function Tile({ label, onPress }: { readonly label: string; readonly onPress: () => void }) {
  const [hovered, setHovered] = useState(false);
  return (
    <Pressable
      onPress={onPress}
      onHoverIn={() => setHovered(true)}
      onHoverOut={() => setHovered(false)}
      style={({ pressed }) => [
        styles.tile,
        hovered && styles.tileHovered,
        pressed && styles.tilePressed,
      ]}
    >
      <Text style={styles.tileLabel}>{label}</Text>
    </Pressable>
  );
}

/** Hover, press and wheel with the remote's pointer. */
export function PointerScreen() {
  const [pressed, setPressed] = useState('nothing yet');
  const [offset, setOffset] = useState(0);
  return (
    <View style={styles.screen}>
      <Text style={styles.title}>Pointer</Text>
      <Text style={styles.hint}>{`Pressed: ${pressed} · list offset: ${Math.round(offset)}`}</Text>
      <View style={styles.row}>
        {TILES.map((label) => (
          <Tile key={label} label={label} onPress={() => setPressed(label)} />
        ))}
      </View>
      <ScrollView
        style={styles.list}
        scrollEventThrottle={16}
        onScroll={(event) => setOffset(event.nativeEvent.contentOffset.y)}
      >
        {ROWS.map((row) => (
          <Tile key={row} label={row} onPress={() => setPressed(row)} />
        ))}
      </ScrollView>
    </View>
  );
}

const styles = StyleSheet.create({
  screen: { flex: 1, backgroundColor: '#0b0f1a', paddingHorizontal: 96, paddingVertical: 56 },
  title: { color: '#ffffff', fontSize: 48, fontWeight: '700' },
  hint: { color: '#7c8db5', fontSize: 24, marginTop: 8 },
  row: { flexDirection: 'row', gap: 24, marginTop: 32 },
  list: { flex: 1, marginTop: 32 },
  tile: {
    flex: 1,
    backgroundColor: '#1a2236',
    borderRadius: 16,
    paddingVertical: 28,
    paddingHorizontal: 32,
    marginBottom: 12,
    borderWidth: 3,
    borderColor: 'transparent',
  },
  tileHovered: { backgroundColor: '#26335a', borderColor: '#ffffff' },
  tilePressed: { backgroundColor: '#3d5afe' },
  tileLabel: { color: '#ffffff', fontSize: 30, fontWeight: '600' },
});
