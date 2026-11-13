import { useRef, useState } from 'react';
import { Animated, ScrollView, StyleSheet, Text, View, useTVEventHandler } from 'react-native';
import Svg, { Defs, LinearGradient, Rect, Stop } from 'react-native-svg';
import { NativeMaskedView } from './native-masked-view.tsx';

const HERO = 691;
const INSET = 104;
const ROOM = 24;
const RAMP = 80;
const ROW = 281;
const HUES = ['#e53935', '#8e24aa', '#3949ab', '#00897b', '#fdd835', '#fb8c00'] as const;

function Ramp() {
  return (
    <Svg width="100%" height={RAMP}>
      <Defs>
        <LinearGradient id="scroll-mask-ramp" x1="0" y1="0" x2="0" y2="1">
          <Stop offset={0} stopColor="#000" stopOpacity={0} />
          <Stop offset={1} stopColor="#000" stopOpacity={1} />
        </LinearGradient>
      </Defs>
      <Rect x="0" y="0" width="100%" height={RAMP} fill="url(#scroll-mask-ramp)" />
    </Svg>
  );
}

function Fill({ children }: { readonly children: React.ReactNode }) {
  return (
    <View collapsable={false} style={StyleSheet.absoluteFill} pointerEvents="none">
      {children}
    </View>
  );
}

/** A page that scrolls under a mask it moves itself: the content fades out below the top inset while the scroll offset, sent back from JavaScript, slides the mask. Down scrolls to the end and Up back, both animated; Right and Left step by 100 points; OK adds three rows. */
export function ScrollMaskScreen() {
  const scroller = useRef<ScrollView>(null);
  const [offset] = useState(() => new Animated.Value(0));
  const [rows, setRows] = useState(3);
  const [frame, setFrame] = useState<{ top: number; height: number } | null>(null);
  const at = useRef(0);
  useTVEventHandler((event) => {
    if (event.eventType === 'down') scroller.current?.scrollToEnd({ animated: true });
    if (event.eventType === 'up') scroller.current?.scrollTo({ y: 0, animated: true });
    if (event.eventType === 'select') setRows((count) => count + 3);
    if (event.eventType === 'right' || event.eventType === 'left') {
      at.current = Math.max(0, at.current + (event.eventType === 'right' ? 100 : -100));
      scroller.current?.scrollTo({ y: at.current, animated: false });
    }
  });
  const mask = frame ? (
    <Animated.View style={{ transform: [{ translateY: Animated.add(offset, INSET - frame.top) }] }}>
      <Ramp />
      <View
        style={{ height: frame.height + Math.max(0, frame.top - INSET), backgroundColor: '#000' }}
      />
    </Animated.View>
  ) : (
    <View style={styles.opaque} />
  );
  return (
    <View style={styles.page}>
      <NativeMaskedView
        style={styles.frame}
        maskElement={
          <Fill>
            <View style={styles.opaque} />
          </Fill>
        }
      >
        <View collapsable={false} style={styles.frameContent}>
          <ScrollView
            ref={scroller}
            scrollEnabled={false}
            scrollEventThrottle={16}
            contentContainerStyle={styles.content}
            onScroll={(event) => offset.setValue(event.nativeEvent.contentOffset.y)}
          >
            <View style={styles.hero}>
              <Text style={styles.heroText}>Hero</Text>
            </View>
            <NativeMaskedView style={styles.masked} maskElement={<Fill>{mask}</Fill>}>
              <View
                collapsable={false}
                style={styles.maskedContent}
                onLayout={(event) => {
                  const height = event.nativeEvent.layout.height;
                  setFrame((was) => (was?.height === height ? was : { top: HERO - ROOM, height }));
                }}
              >
                {Array.from({ length: rows }, (_, index) => (
                  <View
                    key={index}
                    style={[styles.row, { backgroundColor: HUES[index % HUES.length] }]}
                  >
                    <Text style={styles.rowText}>Row {index + 1}</Text>
                  </View>
                ))}
              </View>
            </NativeMaskedView>
          </ScrollView>
        </View>
      </NativeMaskedView>
    </View>
  );
}

const styles = StyleSheet.create({
  page: { flex: 1, backgroundColor: '#101014' },
  frame: { flex: 1, minHeight: 0, marginHorizontal: -ROOM },
  frameContent: { flex: 1, minHeight: 0, paddingHorizontal: ROOM },
  content: { paddingBottom: 40 },
  opaque: { flex: 1, backgroundColor: '#000' },
  hero: { height: HERO, backgroundColor: '#1d3b53', justifyContent: 'flex-end', padding: 64 },
  heroText: { color: '#fff', fontSize: 72, fontWeight: '700' },
  masked: { margin: -ROOM },
  maskedContent: { padding: ROOM },
  row: {
    height: ROW - 26,
    marginTop: 18,
    marginBottom: 8,
    marginHorizontal: 64,
    borderRadius: 12,
    padding: 24,
  },
  rowText: { color: '#fff', fontSize: 40, fontWeight: '700' },
});
