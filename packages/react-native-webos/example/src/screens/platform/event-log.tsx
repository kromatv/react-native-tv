import { useCallback, useState } from 'react';
import { StyleSheet, Text, View } from 'react-native';

const MAX_ENTRIES = 8;

interface Entry {
  readonly id: number;
  readonly text: string;
}

export function useEventLog(scope: string): [readonly Entry[], (entry: string) => void] {
  const [entries, setEntries] = useState<readonly Entry[]>([]);
  const add = useCallback(
    (entry: string) => {
      console.log(`[platform:${scope}] ${entry}`);
      const time = new Date().toTimeString().slice(0, 8);
      setEntries((current) =>
        [{ id: (current[0]?.id ?? 0) + 1, text: `${time}  ${entry}` }, ...current].slice(
          0,
          MAX_ENTRIES,
        ),
      );
    },
    [scope],
  );
  return [entries, add];
}

export function EventLog({ entries }: { readonly entries: readonly Entry[] }) {
  return (
    <View style={styles.log}>
      {entries.length === 0 ? <Text style={styles.empty}>nothing yet</Text> : null}
      {entries.map((entry) => (
        <Text key={entry.id} style={styles.entry} numberOfLines={1}>
          {entry.text}
        </Text>
      ))}
    </View>
  );
}

export function Fact({ label, value }: { readonly label: string; readonly value: string }) {
  return (
    <View style={styles.fact}>
      <Text style={styles.label}>{label}</Text>
      <Text style={styles.value}>{value}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  log: { gap: 6, marginTop: 24 },
  empty: { color: '#7c8db5', fontSize: 22 },
  entry: { color: '#d7def0', fontSize: 22, fontFamily: 'monospace' },
  fact: { flexDirection: 'row', gap: 16, marginBottom: 6 },
  label: { width: 360, color: '#7c8db5', fontSize: 24 },
  value: { flex: 1, color: '#ffffff', fontSize: 24 },
});
