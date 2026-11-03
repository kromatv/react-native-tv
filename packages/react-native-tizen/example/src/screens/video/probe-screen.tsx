import { createVideoPlayer, type VideoPlayer, VideoView } from 'expo-video';
import { useEffect, useState } from 'react';
import { StyleSheet, Text, View } from 'react-native';
import { SAMPLES } from './sources.ts';

const READY_TIMEOUT_MS = 25000;
const TWO_AUDIO_TRACKS = SAMPLES.length - 1;

type Show = (player: VideoPlayer | null, step: string) => void;

function log(message: string, payload?: unknown): void {
  const details = payload === undefined ? '' : ` ${JSON.stringify(payload)}`;
  console.log(`[video-probe] ${message}${details}`);
}

function wait(milliseconds: number): Promise<void> {
  return new Promise((resolve) => setTimeout(resolve, milliseconds));
}

function settled(player: VideoPlayer): Promise<string> {
  if (player.status === 'readyToPlay' || player.status === 'error')
    return Promise.resolve(player.status);
  return new Promise((resolve) => {
    const timer = setTimeout(() => finish('timeout'), READY_TIMEOUT_MS);
    const subscription = player.addListener('statusChange', ({ status, error }) => {
      if (status === 'error') log('error', error);
      if (status === 'readyToPlay' || status === 'error') finish(status);
    });
    function finish(status: string) {
      clearTimeout(timer);
      subscription.remove();
      resolve(status);
    }
  });
}

function describe(player: VideoPlayer) {
  return {
    status: player.status,
    playing: player.playing,
    currentTime: Number(player.currentTime.toFixed(2)),
    duration: player.duration,
    buffered: player.bufferedPosition,
    video: player.videoTrack,
    audio: player.availableAudioTracks.map((track) => track.label),
    audioTrack: player.audioTrack?.id ?? null,
    subtitles: player.availableSubtitleTracks.map((track) => track.label),
    subtitleTrack: player.subtitleTrack?.id ?? null,
  };
}

async function probeSample(index: number, show: Show): Promise<void> {
  const sample = SAMPLES[index]!;
  const player = createVideoPlayer({ uri: sample.uri });
  player.timeUpdateEventInterval = 0.5;
  show(player, sample.name);
  log(`sample ${index + 1}: ${sample.name}`);
  const status = await settled(player);
  log(`sample ${index + 1} settled ${status}`, describe(player));
  if (status === 'readyToPlay') {
    player.play();
    await wait(4000);
    log(`sample ${index + 1} after 4 s of play`, describe(player));
    const subtitles = player.availableSubtitleTracks;
    if (subtitles.length > 0) {
      player.subtitleTrack = subtitles[0]!;
      await wait(6000);
      log(`sample ${index + 1} with subtitles`, describe(player));
    }
  }
  show(null, `${sample.name}: released`);
  player.release();
  await wait(1500);
}

async function probeReady(show: Show): Promise<void> {
  const sample = SAMPLES[TWO_AUDIO_TRACKS]!;
  const player = createVideoPlayer({ uri: sample.uri });
  show(player, 'seek and audio track at READY');
  log('READY probe: prepared, not playing');
  await settled(player);
  const tracks = player.availableAudioTracks;
  log('READY probe before', describe(player));
  if (tracks.length > 1) player.audioTrack = tracks[1]!;
  player.currentTime = 20;
  await wait(3000);
  log('READY probe after seek and select, still READY', describe(player));
  player.play();
  await wait(4000);
  log('READY probe after 4 s of play', describe(player));
  show(null, 'READY probe: released');
  player.release();
  await wait(1500);
}

async function probeTwoPlayers(show: Show): Promise<void> {
  const first = createVideoPlayer({ uri: SAMPLES[0]!.uri });
  first.play();
  show(first, 'two players: the first');
  await settled(first);
  await wait(2000);
  first.pause();
  const second = createVideoPlayer({ uri: SAMPLES[1]!.uri });
  second.play();
  show(second, 'two players: the second, the first paused');
  await settled(second);
  await wait(5000);
  log('two players: first', describe(first));
  log('two players: second', describe(second));
  first.play();
  await wait(3000);
  log('two players: first after play()', describe(first));
  log('two players: second after the first played', describe(second));
  show(null, 'two players: released');
  first.release();
  second.release();
  await wait(1500);
}

export function ProbeScreen() {
  const [player, setPlayer] = useState<VideoPlayer | null>(null);
  const [step, setStep] = useState('starting');

  useEffect(() => {
    const show: Show = (next, label) => {
      setPlayer(next);
      setStep(label);
    };
    void (async () => {
      for (let index = 0; index < SAMPLES.length; index += 1) await probeSample(index, show);
      await probeReady(show);
      await probeTwoPlayers(show);
      log('done');
      show(null, 'done');
    })();
  }, []);

  return (
    <View style={styles.screen}>
      <Text style={styles.title}>{`expo-video probe: ${step}`}</Text>
      <View style={styles.box}>
        {player ? (
          <VideoView player={player} style={StyleSheet.absoluteFill} nativeControls={false} />
        ) : null}
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  screen: { flex: 1 },
  title: {
    position: 'absolute',
    left: 60,
    top: 40,
    color: '#ffffff',
    fontSize: 32,
    backgroundColor: '#0b0f1a',
  },
  box: {
    position: 'absolute',
    left: 320,
    top: 140,
    width: 1280,
    height: 720,
    borderWidth: 4,
    borderColor: '#ffd23f',
  },
});
