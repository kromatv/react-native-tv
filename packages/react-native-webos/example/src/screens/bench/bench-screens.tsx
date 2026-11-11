import { useCallback, useState } from 'react';
import { StyleSheet, Text, View, useTVEventHandler } from 'react-native';
import { AnimationBench } from './animation-bench.tsx';
import { useFirstFrameReport, useMemoryReport } from './bench-report.ts';
import { SPEEDS_MS, TIMELINE, useBenchTimeline, type BenchState } from './bench-timeline.ts';
import { ListBench } from './list-bench.tsx';

const TITLES = ['FlatList', 'Native Animated driver'] as const;

function MemoryLabel() {
  const memory = useMemoryReport(1000);
  const text =
    memory === null
      ? 'RSS: reading…'
      : `RSS ${memory.rssMb.toFixed(1)} MB · peak ${memory.peakRssMb.toFixed(1)} MB`;
  return <Text style={styles.memory}>{text}</Text>;
}

function nextSpeed(speed: number): number {
  return speed + 1 < SPEEDS_MS.length ? speed + 1 : -1;
}

export function BenchScreens() {
  useFirstFrameReport();
  const [state, setState] = useState<BenchState>({ screen: 0, speed: -1 });
  const apply = useCallback((next: BenchState) => setState(next), []);
  useBenchTimeline(apply);
  useTVEventHandler((event) => {
    if (TIMELINE) return;
    if (event.eventType === 'right') setState((current) => ({ ...current, screen: 1 }));
    if (event.eventType === 'left') setState((current) => ({ ...current, screen: 0 }));
    if (event.eventType === 'select')
      setState((current) => ({ ...current, speed: nextSpeed(current.speed) }));
  });
  const hint = TIMELINE
    ? 'Running the benchmark timeline: the remote does nothing'
    : 'Left and Right switch screens · Enter changes the list speed';
  return (
    <View style={styles.screen}>
      <View style={styles.header}>
        <Text style={styles.title}>{TITLES[state.screen]}</Text>
        <MemoryLabel />
      </View>
      <Text style={styles.hint}>{`${state.screen + 1} of ${TITLES.length} · ${hint}`}</Text>
      <View style={styles.body}>
        {state.screen === 0 ? <ListBench step={SPEEDS_MS[state.speed]} /> : <AnimationBench />}
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  screen: { flex: 1, backgroundColor: '#0b0f1a', paddingHorizontal: 96, paddingVertical: 56 },
  header: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'baseline' },
  title: { color: '#ffffff', fontSize: 48, fontWeight: '700' },
  memory: { color: '#ffb300', fontSize: 26 },
  hint: { color: '#7c8db5', fontSize: 24, marginTop: 8 },
  body: { flex: 1, marginTop: 32 },
});
