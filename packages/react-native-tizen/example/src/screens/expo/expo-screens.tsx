import { useState } from 'react';
import { StyleSheet, Text, View, useTVEventHandler } from 'react-native';
import { DeviceScreen } from './device-screen.tsx';
import { FileSystemScreen } from './file-system-screen.tsx';
import { FontScreen } from './font-screen.tsx';
import { RuntimeScreen } from './runtime-screen.tsx';
import { ViewsScreen } from './views-screen.tsx';

const SCREENS = [
  { title: 'Expo runtime', Screen: RuntimeScreen },
  { title: 'Device, constants, keep awake', Screen: DeviceScreen },
  { title: 'File system', Screen: FileSystemScreen },
  { title: 'Fonts', Screen: FontScreen },
  { title: 'expo-image and expo-blur', Screen: ViewsScreen },
] as const;

export function ExpoScreens() {
  const [index, setIndex] = useState(0);
  useTVEventHandler((event) => {
    if (event.eventType === 'right')
      setIndex((current) => Math.min(SCREENS.length - 1, current + 1));
    if (event.eventType === 'left') setIndex((current) => Math.max(0, current - 1));
  });
  const { title, Screen } = SCREENS[index] ?? SCREENS[0];
  return (
    <View style={styles.screen}>
      <Text style={styles.title}>{title}</Text>
      <Text
        style={styles.hint}
      >{`${index + 1} of ${SCREENS.length} · Left and Right switch screens`}</Text>
      <View style={styles.body}>
        <Screen />
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  screen: { flex: 1, backgroundColor: '#0b0f1a', paddingHorizontal: 96, paddingVertical: 56 },
  title: { color: '#ffffff', fontSize: 48, fontWeight: '700' },
  hint: { color: '#7c8db5', fontSize: 24, marginTop: 8 },
  body: { flex: 1, marginTop: 32 },
});
