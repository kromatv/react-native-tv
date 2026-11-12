import { useEffect, useState } from 'react';
import { Animated, Easing, StyleSheet, Text, View } from 'react-native';

const BOX_COUNT = 12;
const TRAVEL = 1400;
const CYCLE_MS = 3000;
const STAGGER_MS = 120;

export function AnimationBench() {
  const [lanes] = useState(() =>
    Array.from({ length: BOX_COUNT }, (_, index) => ({
      id: `lane-${index}`,
      value: new Animated.Value(0),
    })),
  );
  useEffect(() => {
    const loops = lanes.map(({ value }) =>
      Animated.loop(
        Animated.timing(value, {
          toValue: 1,
          duration: CYCLE_MS,
          easing: Easing.inOut(Easing.sin),
          useNativeDriver: true,
        }),
      ),
    );
    const all = Animated.stagger(STAGGER_MS, loops);
    all.start();
    return () => all.stop();
  }, [lanes]);
  return (
    <View style={styles.body}>
      <Text style={styles.hint}>
        {`${BOX_COUNT} views, translateX and opacity, useNativeDriver: true, looping every ${CYCLE_MS / 1000} s`}
      </Text>
      <View style={styles.lanes}>
        {lanes.map(({ id, value }) => (
          <Animated.View
            key={id}
            style={[
              styles.box,
              {
                opacity: value.interpolate({ inputRange: [0, 0.5, 1], outputRange: [1, 0.4, 1] }),
                transform: [
                  {
                    translateX: value.interpolate({
                      inputRange: [0, 0.5, 1],
                      outputRange: [0, TRAVEL, 0],
                    }),
                  },
                ],
              },
            ]}
          />
        ))}
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  body: { flex: 1 },
  hint: { color: '#a9b4cf', fontSize: 24 },
  lanes: { marginTop: 24, gap: 16 },
  box: { width: 120, height: 52, borderRadius: 12, backgroundColor: '#69f0ae' },
});
