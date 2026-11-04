import { memo, useEffect, useRef, useState } from 'react';
import { FlatList, StyleSheet, Text, View, useTVEventHandler } from 'react-native';
import { TIMELINE } from './bench-timeline.ts';

const ROW_COUNT = 1000;
const ROW_HEIGHT = 96;
const LIST_HEIGHT = 780;

interface Row {
  readonly id: string;
  readonly title: string;
  readonly detail: string;
  readonly color: string;
}

const ROWS: readonly Row[] = Array.from({ length: ROW_COUNT }, (_, index) => ({
  id: String(index),
  title: `Title ${index + 1}`,
  detail: `${1950 + (index % 75)} · ${80 + (index % 70)} min · Drama`,
  color: `hsl(${(index * 37) % 360}, 55%, 42%)`,
}));

const BenchRow = memo(function BenchRow({
  row,
  focused,
}: {
  readonly row: Row;
  readonly focused: boolean;
}) {
  return (
    <View style={[styles.row, focused && styles.rowFocused]}>
      <View style={[styles.tile, { backgroundColor: row.color }]} />
      <View>
        <Text style={styles.title}>{row.title}</Text>
        <Text style={styles.detail}>{row.detail}</Text>
      </View>
    </View>
  );
});

function rowLayout(_: ArrayLike<Row> | null | undefined, index: number) {
  return { length: ROW_HEIGHT, offset: ROW_HEIGHT * index, index };
}

export function ListBench({ step }: { readonly step: number | undefined }) {
  const list = useRef<FlatList<Row>>(null);
  const [focused, setFocused] = useState(0);
  const motion = step === undefined ? 'still' : `moves one row every ${step} ms`;
  useTVEventHandler((event) => {
    if (TIMELINE) return;
    if (event.eventType === 'down') setFocused((index) => Math.min(ROW_COUNT - 1, index + 1));
    if (event.eventType === 'up') setFocused((index) => Math.max(0, index - 1));
  });
  useEffect(() => {
    if (step === undefined) return;
    const timer = setInterval(() => setFocused((index) => (index + 1) % ROW_COUNT), step);
    return () => clearInterval(timer);
  }, [step]);
  useEffect(() => {
    list.current?.scrollToIndex({ index: focused, animated: focused > 0, viewPosition: 0.5 });
  }, [focused]);
  return (
    <View style={styles.body}>
      <Text style={styles.hint}>{`${ROW_COUNT} rows · focus ${motion} · row ${focused + 1}`}</Text>
      <FlatList
        ref={list}
        data={ROWS}
        keyExtractor={(row) => row.id}
        getItemLayout={rowLayout}
        extraData={focused}
        renderItem={({ item, index }) => <BenchRow row={item} focused={index === focused} />}
        style={styles.list}
      />
    </View>
  );
}

const styles = StyleSheet.create({
  body: { flex: 1 },
  hint: { color: '#a9b4cf', fontSize: 24 },
  list: { height: LIST_HEIGHT, marginTop: 24 },
  row: {
    height: ROW_HEIGHT,
    flexDirection: 'row',
    alignItems: 'center',
    gap: 24,
    paddingHorizontal: 24,
    borderRadius: 16,
  },
  rowFocused: { backgroundColor: '#3d5afe' },
  tile: { width: 112, height: 64, borderRadius: 8 },
  title: { color: '#ffffff', fontSize: 30, fontWeight: '600' },
  detail: { color: '#a9b4cf', fontSize: 22, marginTop: 4 },
});
