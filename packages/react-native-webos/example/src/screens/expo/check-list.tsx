import { useEffect, useState } from 'react';
import { StyleSheet, Text, View } from 'react-native';

export interface Check {
  readonly label: string;
  readonly value: string;
  readonly ok: boolean;
}

export function check(label: string, ok: boolean, value: unknown): Check {
  return {
    label,
    ok,
    value: typeof value === 'string' ? value : (JSON.stringify(value) ?? String(value)),
  };
}

export async function attempt(label: string, run: () => Promise<Check> | Check): Promise<Check> {
  try {
    return await run();
  } catch (error) {
    return check(label, false, error instanceof Error ? error.message : String(error));
  }
}

export function useChecks(
  name: string,
  run: () => Promise<readonly Check[]>,
  scope = 'expo',
): readonly Check[] | null {
  const [checks, setChecks] = useState<readonly Check[] | null>(null);
  useEffect(() => {
    let alive = true;
    void run().then((results) => {
      for (const result of results) {
        console.log(
          `[${scope}:${name}] ${result.ok ? 'PASS' : 'FAIL'} ${result.label}: ${result.value}`,
        );
      }
      if (alive) setChecks(results);
    });
    return () => {
      alive = false;
    };
  }, [name, run, scope]);
  return checks;
}

export function CheckList({ checks }: { readonly checks: readonly Check[] | null }) {
  if (checks === null) return <Text style={styles.pending}>running…</Text>;
  return (
    <View style={styles.list}>
      {checks.map((item) => (
        <View key={item.label} style={styles.row}>
          <Text style={[styles.badge, item.ok ? styles.pass : styles.fail]}>
            {item.ok ? 'PASS' : 'FAIL'}
          </Text>
          <Text style={styles.label}>{item.label}</Text>
          <Text style={styles.value} numberOfLines={1}>
            {item.value}
          </Text>
        </View>
      ))}
    </View>
  );
}

const styles = StyleSheet.create({
  list: { gap: 10 },
  row: { flexDirection: 'row', alignItems: 'center', gap: 20 },
  badge: {
    width: 90,
    fontSize: 22,
    fontWeight: '700',
    textAlign: 'center',
    borderRadius: 8,
    paddingVertical: 4,
  },
  pass: { color: '#0b0f1a', backgroundColor: '#69f0ae' },
  fail: { color: '#ffffff', backgroundColor: '#ff5252' },
  label: { width: 520, color: '#ffffff', fontSize: 24 },
  value: { flex: 1, color: '#a9b4cf', fontSize: 22 },
  pending: { color: '#a9b4cf', fontSize: 28 },
});
