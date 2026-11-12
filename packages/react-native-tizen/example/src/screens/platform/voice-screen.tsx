import { Voice } from '@kromatv/react-native-tizen/voice';
import { useVideoPlayer, VideoView } from 'expo-video';
import { useEffect, useState } from 'react';
import {
  Linking,
  Platform,
  Pressable,
  StyleSheet,
  Text,
  View,
  useTVEventHandler,
} from 'react-native';
import { SAMPLES } from '../video/sources.ts';
import { EventLog, Fact, useEventLog } from './event-log.tsx';

const TITLES = ['Blade Runner', 'Dune', 'Arrival', 'Sicario'] as const;

export function VoiceScreen() {
  const [entries, add] = useEventLog('voice');
  const [highlight, setHighlight] = useState(0);
  const [commandsToJs, setCommandsToJs] = useState(false);
  const [available, setAvailable] = useState(Voice.isAvailable);
  const player = useVideoPlayer(SAMPLES[0]?.uri ?? null, (created) => {
    created.loop = true;
  });

  useEffect(() => {
    const timer = setTimeout(() => {
      setAvailable(Voice.isAvailable);
      add(
        `Platform.Version ${Platform.Version}, unavailableReason ${String(Voice.unavailableReason)}, canSelectTitles ${String(Voice.canSelectTitles)}`,
      );
    }, 2000);
    return () => clearTimeout(timer);
  }, [add]);

  useEffect(() => {
    Voice.setScreen('list');
    const search = Voice.addListener('search', (query) => add(`search ${JSON.stringify(query)}`));
    const url = Linking.addEventListener('url', (event) => add(`Linking url ${event.url}`));
    return () => {
      Voice.setScreen(null);
      search.remove();
      url.remove();
    };
  }, [add]);

  useEffect(() => {
    if (!commandsToJs) return undefined;
    const command = Voice.addListener('command', (value) =>
      add(`command ${JSON.stringify(value)}`),
    );
    return () => command.remove();
  }, [commandsToJs, add]);

  useTVEventHandler((event) => {
    if (event.eventKeyAction !== 1) return;
    if (event.eventType === 'down') setHighlight((index) => Math.min(TITLES.length - 1, index + 1));
    if (event.eventType === 'up') setHighlight((index) => Math.max(0, index - 1));
    if (event.eventType === 'select') add(`remote pressed ${TITLES[highlight] ?? ''}`);
    if (event.eventType === 'playPause') setCommandsToJs((current) => !current);
  });

  return (
    <View style={styles.columns}>
      <View style={styles.column}>
        <Fact label="Voice.isAvailable" value={String(available)} />
        <Fact label="Voice.unavailableReason" value={String(Voice.unavailableReason)} />
        <Fact label="Voice.canSelectTitles" value={String(Voice.canSelectTitles)} />
        <Fact label="Platform.Version" value={String(Platform.Version)} />
        <Fact
          label="Transport commands go to"
          value={
            commandsToJs ? 'JavaScript (Play/Pause toggles)' : 'the player (Play/Pause toggles)'
          }
        />
        <View style={styles.titles}>
          {TITLES.map((title, index) => (
            <Pressable
              key={title}
              accessibilityLabel={title}
              onPress={() => add(`voice pressed ${title}`)}
              style={[styles.title, index === highlight ? styles.highlighted : null]}
            >
              <Text style={styles.titleText}>{title}</Text>
            </Pressable>
          ))}
        </View>
        <EventLog entries={entries} />
      </View>
      <VideoView player={player} style={styles.video} nativeControls={false} />
    </View>
  );
}

const styles = StyleSheet.create({
  columns: { flex: 1, flexDirection: 'row', gap: 48 },
  column: { flex: 1 },
  titles: { flexDirection: 'row', gap: 16, marginTop: 16 },
  title: {
    paddingHorizontal: 24,
    paddingVertical: 16,
    borderRadius: 12,
    backgroundColor: '#1b2340',
  },
  highlighted: { backgroundColor: '#3d5afe' },
  titleText: { color: '#ffffff', fontSize: 26 },
  video: { width: 640, height: 360, backgroundColor: '#000000' },
});
