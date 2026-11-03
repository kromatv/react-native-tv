import { useMemo } from 'react';
import { ScrollView, StyleSheet, Text, View } from 'react-native';
import { intlCases } from './intl-cases.ts';

function timed<T>(make: () => T): [T, number] {
  const start = performance.now();
  const value = make();
  return [value, performance.now() - start];
}

/** Intl on Tizen: the calls KROMA makes, each with what it returned, logged for a diff against Node. */
export function IntlScreen() {
  const [cases, elapsed] = useMemo(() => timed(intlCases), []);
  useMemo(() => {
    console.log(`INTL ${JSON.stringify(cases)}`);
    console.log(
      `INTL first run ${elapsed.toFixed(1)} ms, second ${timed(intlCases)[1].toFixed(1)} ms`,
    );
  }, [cases, elapsed]);
  return (
    <View style={styles.screen}>
      <Text style={styles.title}>Intl, in French and English</Text>
      <ScrollView>
        {cases.map((entry) => (
          <View key={entry.name} style={styles.row}>
            <Text style={styles.name}>{entry.name}</Text>
            <Text style={styles.value}>{entry.value}</Text>
          </View>
        ))}
      </ScrollView>
    </View>
  );
}

const styles = StyleSheet.create({
  screen: { flex: 1, backgroundColor: '#0b0f1a', paddingHorizontal: 96, paddingVertical: 48 },
  title: { color: '#ffffff', fontSize: 40, fontWeight: '700', marginBottom: 16 },
  row: { flexDirection: 'row', paddingVertical: 2 },
  name: { color: '#8b93a7', fontSize: 18, width: 340 },
  value: { color: '#ffffff', fontSize: 18, flex: 1 },
});
