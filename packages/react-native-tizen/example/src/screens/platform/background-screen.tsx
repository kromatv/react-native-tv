import { useVideoPlayer, VideoView, type VideoPlayer } from 'expo-video';
import { useEffect, useState } from 'react';
import { AppState, StyleSheet, View, useTVEventHandler } from 'react-native';
import { SAMPLES } from '../video/sources.ts';
import { EventLog, Fact, useEventLog } from './event-log.tsx';

const REPORT_EVERY_MS = 5000;

function setStaysActiveInBackground(player: VideoPlayer, staysActive: boolean) {
  player.staysActiveInBackground = staysActive;
}

export function BackgroundScreen() {
  const [entries, add] = useEventLog('background');
  const [staysActive, setStaysActive] = useState(false);
  const player = useVideoPlayer(SAMPLES[0]?.uri ?? null, (created) => {
    created.loop = true;
    created.play();
  });

  const toggleStaysActive = () => {
    const next = !staysActive;
    setStaysActive(next);
    setStaysActiveInBackground(player, next);
    add(`staysActiveInBackground ${next}`);
  };

  useEffect(() => {
    const report = () =>
      `appState ${AppState.currentState}, playing ${player.playing}, at ${player.currentTime.toFixed(1)} s`;
    const changes = AppState.addEventListener('change', () => add(report()));
    const timer = setInterval(
      () => console.log(`[platform:background] ${report()}`),
      REPORT_EVERY_MS,
    );
    return () => {
      changes.remove();
      clearInterval(timer);
    };
  }, [player, add]);

  useTVEventHandler((event) => {
    if (event.eventKeyAction !== 1) return;
    if (event.eventType === 'select') toggleStaysActive();
    if (event.eventType === 'playPause' || event.eventType === 'play') {
      player.play();
      add('play');
    }
  });

  return (
    <View>
      <Fact label="staysActiveInBackground" value={`${staysActive} (Enter toggles, Play plays)`} />
      <Fact label="Try" value="press Home, wait, come back: the log says what played" />
      <VideoView player={player} style={styles.video} nativeControls={false} />
      <EventLog entries={entries} />
    </View>
  );
}

const styles = StyleSheet.create({
  video: { width: 640, height: 360, backgroundColor: '#000000', marginTop: 16 },
});
