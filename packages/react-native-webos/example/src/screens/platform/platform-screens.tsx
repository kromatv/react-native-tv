import { useState } from 'react';
import { StyleSheet, Text, View, useTVEventHandler } from 'react-native';
import { AlertsScreen } from './alerts-screen.tsx';
import { BackgroundScreenOrNotice } from './background-screen-or-notice.tsx';
import { DeepLinkScreen } from './deep-link-screen.tsx';
import { DeviceScreen } from './device-screen.tsx';
import { LifecycleScreen } from './lifecycle-screen.tsx';
import { ScreenSizeScreen } from './screen-size-screen.tsx';

const SCREENS = [
  { title: 'Lifecycle', Screen: LifecycleScreen },
  { title: 'Device', Screen: DeviceScreen },
  { title: 'Deep links', Screen: DeepLinkScreen },
  { title: 'Alerts and errors', Screen: AlertsScreen },
  { title: 'Screen and 4K', Screen: ScreenSizeScreen },
  { title: 'Background audio', Screen: BackgroundScreenOrNotice },
] as const;

export function PlatformScreens() {
  const [index, setIndex] = useState(0);
  useTVEventHandler((event) => {
    if (event.eventKeyAction !== 1) return;
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
