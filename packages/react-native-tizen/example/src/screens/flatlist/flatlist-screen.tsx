import { FlatList, StyleSheet, Text, View } from 'react-native';
import { ScreenFrame, palette } from '../rendering/screen-frame.tsx';

const ROWS = Array.from({ length: 40 }, (_, index) => `Row ${index + 1}`);
const CARDS = Array.from({ length: 12 }, (_, index) => `Card ${index + 1}`);

function Row({ label }: { readonly label: string }) {
  return (
    <View style={styles.row}>
      <Text style={styles.label}>{label}</Text>
    </View>
  );
}

function Card({ label }: { readonly label: string }) {
  return (
    <View style={styles.card}>
      <Text style={styles.label}>{label}</Text>
    </View>
  );
}

export function FlatListScreen() {
  return (
    <ScreenFrame
      title="FlatList sized by flex: 1"
      hint="Each list has style={{ flex: 1 }} and no height: it fills what its parent leaves"
    >
      <View style={styles.columns}>
        <View style={styles.column}>
          <Text style={styles.caption}>Vertical, between a caption and a footer</Text>
          <FlatList
            data={ROWS}
            keyExtractor={(item) => item}
            renderItem={({ item }) => <Row label={item} />}
            style={styles.fill}
          />
          <Text style={styles.footer}>Footer: below the list</Text>
        </View>
        <View style={styles.column}>
          <Text style={styles.caption}>Horizontal, in a 160 px row</Text>
          <View style={styles.strip}>
            <FlatList
              horizontal
              data={CARDS}
              keyExtractor={(item) => item}
              renderItem={({ item }) => <Card label={item} />}
              style={styles.fill}
            />
          </View>
        </View>
      </View>
    </ScreenFrame>
  );
}

const styles = StyleSheet.create({
  columns: { flex: 1, flexDirection: 'row', gap: 48 },
  column: { flex: 1, borderWidth: 2, borderColor: palette.accent, padding: 16 },
  caption: { color: palette.muted, fontSize: 24, marginBottom: 12 },
  fill: { flex: 1 },
  footer: { color: palette.amber, fontSize: 28, marginTop: 12 },
  strip: { height: 160, flexDirection: 'row' },
  row: {
    height: 64,
    justifyContent: 'center',
    paddingHorizontal: 20,
    marginBottom: 8,
    borderRadius: 12,
    backgroundColor: palette.card,
  },
  card: {
    width: 220,
    marginRight: 16,
    justifyContent: 'center',
    alignItems: 'center',
    borderRadius: 16,
    backgroundColor: palette.card,
  },
  label: { color: palette.text, fontSize: 26 },
});
