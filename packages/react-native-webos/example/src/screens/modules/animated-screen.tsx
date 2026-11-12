import { useEffect, useState } from 'react';
import { Animated, Easing, StyleSheet, Text, View, useTVEventHandler } from 'react-native';

const DURATION_MS = 6000;
const BLOCK_AFTER_MS = 1500;
const BLOCK_FOR_MS = 3000;
const TRAVEL = 1300;

function blockJavaScript(milliseconds: number): number {
  const end = Date.now() + milliseconds;
  let spins = 0;
  while (Date.now() < end) spins += 1;
  return spins;
}

function timing(value: Animated.Value, useNativeDriver: boolean): Animated.CompositeAnimation {
  return Animated.timing(value, {
    toValue: 1,
    duration: DURATION_MS,
    easing: Easing.linear,
    useNativeDriver,
  });
}

export function AnimatedScreen() {
  const [native] = useState(() => new Animated.Value(0));
  const [javascript] = useState(() => new Animated.Value(0));
  const [run, setRun] = useState(0);
  useTVEventHandler((event) => {
    if (event.eventType === 'select') setRun((current) => current + 1);
  });
  useEffect(() => {
    native.setValue(0);
    javascript.setValue(0);
    const started = Date.now();
    const log = (message: string) =>
      console.log(`[modules:animated] run ${run} +${Date.now() - started} ms: ${message}`);
    log('started');
    const animations = [timing(native, true), timing(javascript, false)];
    animations[0]?.start(({ finished }) => log(`native driver finished=${finished}`));
    animations[1]?.start(({ finished }) => log(`JavaScript driver finished=${finished}`));
    const block = setTimeout(() => {
      log(`blocking JavaScript for ${BLOCK_FOR_MS} ms`);
      const spins = blockJavaScript(BLOCK_FOR_MS);
      log(`JavaScript free again after ${spins} spins`);
    }, BLOCK_AFTER_MS);
    return () => {
      clearTimeout(block);
      for (const animation of animations) animation.stop();
    };
  }, [run, native, javascript]);

  const moving = (value: Animated.Value) => ({
    transform: [
      { translateX: value.interpolate({ inputRange: [0, 1], outputRange: [0, TRAVEL] }) },
    ],
  });
  return (
    <View style={styles.body}>
      <Text style={styles.hint}>
        {`Both boxes move for ${DURATION_MS / 1000} s; JavaScript is blocked from ${BLOCK_AFTER_MS / 1000} s to ${(BLOCK_AFTER_MS + BLOCK_FOR_MS) / 1000} s. Enter runs it again.`}
      </Text>
      <Text style={styles.label}>useNativeDriver: true</Text>
      <Animated.View
        style={[
          styles.box,
          styles.native,
          moving(native),
          { opacity: native.interpolate({ inputRange: [0, 0.5, 1], outputRange: [1, 0.35, 1] }) },
        ]}
      />
      <Text style={styles.label}>useNativeDriver: false</Text>
      <Animated.View style={[styles.box, styles.javascript, moving(javascript)]} />
    </View>
  );
}

const styles = StyleSheet.create({
  body: { gap: 24 },
  hint: { color: '#a9b4cf', fontSize: 24 },
  label: { color: '#ffffff', fontSize: 28, marginTop: 24 },
  box: { width: 160, height: 160, borderRadius: 24 },
  native: { backgroundColor: '#69f0ae' },
  javascript: { backgroundColor: '#ff8a65' },
});
