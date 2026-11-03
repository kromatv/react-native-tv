import { StyleSheet, Text, View } from 'react-native';
import { FocusRow } from './focus-row.tsx';
import { ShapeSamples } from './shape-samples.tsx';
import { TablerGrid } from './tabler-grid.tsx';

/** react-native-svg on Tizen: Tabler's icons, each element on its own, then live updates. */
export function SvgScreen() {
  return (
    <View style={styles.screen}>
      <Text style={styles.title}>react-native-svg, drawn with Evas VG</Text>
      <TablerGrid />
      <Text style={styles.heading}>Elements</Text>
      <ShapeSamples />
      <Text style={styles.heading}>Updates</Text>
      <FocusRow />
    </View>
  );
}

const styles = StyleSheet.create({
  screen: { flex: 1, backgroundColor: '#0b0f1a', paddingHorizontal: 96, paddingVertical: 56 },
  title: { color: '#ffffff', fontSize: 44, fontWeight: '700', marginBottom: 24 },
  heading: { color: '#ffffff', fontSize: 32, fontWeight: '600', marginTop: 32, marginBottom: 16 },
});
