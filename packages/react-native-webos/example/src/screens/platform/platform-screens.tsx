import { SmartHubPreview } from '@kromatv/react-native-tizen/preview';
import { useEffect, useState } from 'react';
import { Alert, BackHandler, StyleSheet, Text, View, useTVEventHandler } from 'react-native';
import { BackgroundScreen } from './background-screen.tsx';
import { PreviewScreen } from './preview-screen.tsx';
import { ScreenSizeScreen } from './screen-size-screen.tsx';
import { VoiceScreen } from './voice-screen.tsx';

const SCREENS = [
  { title: 'Voice', Screen: VoiceScreen },
  { title: 'Smart Hub preview', Screen: PreviewScreen },
  { title: 'Background audio', Screen: BackgroundScreen },
  { title: 'Screen and 4K', Screen: ScreenSizeScreen },
] as const;

function useExitOnBack() {
  useEffect(() => {
    const subscription = BackHandler.addEventListener('hardwareBackPress', () => {
      if (SmartHubPreview.launchedFromTile) {
        console.log('[platform:back] launched from a tile: exiting with no popup');
        return false;
      }
      Alert.alert('Leave the example?', undefined, [
        { text: 'Stay', style: 'cancel' },
        { text: 'Leave', onPress: () => BackHandler.exitApp() },
      ]);
      return true;
    });
    return () => subscription.remove();
  }, []);
}

export function PlatformScreens() {
  const [index, setIndex] = useState(0);
  useExitOnBack();
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
