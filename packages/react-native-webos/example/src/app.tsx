import { requireOptionalNativeModule } from 'expo';
import { useEffect, useState } from 'react';
import { BackHandler, Platform, StyleSheet, Text, View, useTVEventHandler } from 'react-native';
import { RenderingScreen, useRenderingScreen } from './screens/rendering/rendering-screens.tsx';

interface ExpoTizenModule {
  readonly modelName: string;
  readonly platformVersion: string;
}

const expoTizen = requireOptionalNativeModule<ExpoTizenModule>('ExpoTizen');

const PILLARS = [
  { name: 'Hermes', detail: 'bytecode, no browser' },
  { name: 'Fabric', detail: 'the new renderer' },
  { name: 'Expo', detail: 'same project, one more platform' },
] as const;

function useSecondsSinceMount(): number {
  const [seconds, setSeconds] = useState(0);
  useEffect(() => {
    const timer = setInterval(() => setSeconds((value) => value + 1), 1000);
    return () => clearInterval(timer);
  }, []);
  return seconds;
}

function useFetchedText(url: string): string {
  const [text, setText] = useState('fetching…');
  useEffect(() => {
    fetch(url)
      .then((response) => response.text())
      .then((body) => {
        console.log(`fetch ${url}: ${body}`);
        setText(body.trim());
      })
      .catch((error: unknown) => {
        console.error(`fetch ${url} failed: ${String(error)}`);
        setText(`fetch failed: ${String(error)}`);
      });
  }, [url]);
  return text;
}

function useRemoteSelection(count: number): { selected: number; lastKey: string } {
  const [selected, setSelected] = useState(0);
  const [lastKey, setLastKey] = useState('none yet');
  useTVEventHandler((event) => {
    console.log(`remote: ${event.eventType}`);
    setLastKey(event.eventType);
    if (event.eventType === 'right') setSelected((index) => Math.min(count - 1, index + 1));
    if (event.eventType === 'left') setSelected((index) => Math.max(0, index - 1));
  });
  useEffect(() => {
    const subscription = BackHandler.addEventListener('hardwareBackPress', () => {
      console.log('remote: back');
      setLastKey('back');
      return false;
    });
    return () => subscription.remove();
  }, []);
  return { selected, lastKey };
}

export function App() {
  const { selected, lastKey } = useRemoteSelection(PILLARS.length);
  const seconds = useSecondsSinceMount();
  const zen = useFetchedText('https://api.github.com/zen');
  const renderingScreen = useRenderingScreen();
  if (renderingScreen !== null) return <RenderingScreen index={renderingScreen} />;
  return (
    <View style={styles.screen}>
      <Text style={styles.eyebrow}>
        {expoTizen
          ? `${Platform.OS} · ${expoTizen.modelName} · Tizen ${expoTizen.platformVersion}`
          : `${Platform.OS} · no ExpoTizen module`}
      </Text>
      <Text style={styles.title}>React Native, native on a Samsung TV</Text>
      <View style={styles.row}>
        {PILLARS.map((pillar, index) => (
          <View key={pillar.name} style={[styles.card, index === selected && styles.cardSelected]}>
            <Text style={styles.cardTitle}>{pillar.name}</Text>
            <Text style={styles.cardDetail}>{pillar.detail}</Text>
          </View>
        ))}
      </View>
      <Text style={styles.fetched}>{`GitHub says: ${zen}`}</Text>
      <Text style={styles.footer}>{`Up for ${seconds}s · last remote key: ${lastKey}`}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  screen: { flex: 1, backgroundColor: '#0b0f1a', paddingHorizontal: 96, paddingVertical: 72 },
  eyebrow: { color: '#7c8db5', fontSize: 24, letterSpacing: 2 },
  title: { color: '#ffffff', fontSize: 56, fontWeight: '700', marginTop: 12 },
  row: { flexDirection: 'row', gap: 32, marginTop: 64 },
  card: { flex: 1, backgroundColor: '#1a2236', borderRadius: 24, padding: 40 },
  cardSelected: { backgroundColor: '#3d5afe' },
  cardTitle: { color: '#ffffff', fontSize: 40, fontWeight: '600' },
  cardDetail: { color: '#a9b4cf', fontSize: 26, marginTop: 12 },
  fetched: { color: '#d7def0', fontSize: 30, marginTop: 64 },
  footer: { color: '#7c8db5', fontSize: 28, marginTop: 'auto' },
});
