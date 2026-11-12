import { StyleSheet, Text, View } from 'react-native';
import { FocusRow } from './focus-row.tsx';
import { ShapeSamples } from './shape-samples.tsx';
import { SkiaSamples } from './skia-samples.tsx';
import { TablerGrid } from './tabler-grid.tsx';

/** react-native-svg on webOS: Tabler's icons, each element on its own, then live updates. */
export function SvgScreen() {
  return (
    <View style={styles.screen}>
      <Text style={styles.title}>react-native-svg, drawn with Skia</Text>
      <TablerGrid />
      <Text style={styles.heading}>Elements</Text>
      <ShapeSamples />
      <SkiaSamples />
      <Text style={styles.heading}>Updates</Text>
      <FocusRow />
    </View>
  );
}

const styles = StyleSheet.create({
  screen: { flex: 1, backgroundColor: '#0b0f1a', paddingHorizontal: 96, paddingVertical: 40 },
  title: { color: '#ffffff', fontSize: 44, fontWeight: '700', marginBottom: 12 },
  heading: { color: '#ffffff', fontSize: 32, fontWeight: '600', marginTop: 20, marginBottom: 12 },
});
