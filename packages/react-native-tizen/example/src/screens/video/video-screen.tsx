import {
  createVideoPlayer,
  type VideoContentFit,
  type VideoPlayer,
  type VideoTrack,
  VideoView,
} from 'expo-video';
import { useEffect, useRef, useState } from 'react';
import { StyleSheet, Text, View, useTVEventHandler } from 'react-native';
import { SAMPLES } from './sources.ts';
import { usePlayerReport } from './use-player-report.ts';

const FITS: readonly VideoContentFit[] = ['contain', 'cover', 'fill'];
const ACTIONS = [
  'Backdrop behind the video',
  'Content fit',
  'Box size',
  'Play / pause',
  'Seek +30 s',
  'Seek -10 s',
  'Next audio track',
  'Next subtitle track, then off',
  'Fullscreen',
  'Probe seek and audio track at READY',
  'Swap to a new player',
] as const;

function openPlayer(uri: string): VideoPlayer {
  const player = createVideoPlayer({ uri });
  player.timeUpdateEventInterval = 1;
  player.play();
  return player;
}

function selectNextAudioTrack(player: VideoPlayer): void {
  const tracks = player.availableAudioTracks;
  const index = tracks.findIndex((track) => track.id === player.audioTrack?.id);
  if (tracks.length > 0) player.audioTrack = tracks[(index + 1) % tracks.length]!;
}

function selectNextSubtitle(player: VideoPlayer): void {
  const tracks = player.availableSubtitleTracks;
  const index = tracks.findIndex((track) => track.id === player.subtitleTrack?.id);
  player.subtitleTrack = index + 1 < tracks.length ? tracks[index + 1]! : null;
}

function probeAtReady(player: VideoPlayer): void {
  const tracks = player.availableAudioTracks;
  console.log(`[video-screen] probe at READY: seek to 30 s, ${tracks.length} audio tracks`);
  player.currentTime = 30;
  if (tracks.length > 1) player.audioTrack = tracks[1]!;
  setTimeout(() => {
    console.log(`[video-screen] probe: play, currentTime ${player.currentTime.toFixed(2)}`);
    player.play();
  }, 3000);
}

function describeVideo(video: VideoTrack | null): string {
  if (!video) return 'unknown';
  const size = `${video.size.width}x${video.size.height}`;
  return `${size} ${video.mimeType ?? '?'} ${video.frameRate ?? '?'} fps`;
}

function next<T>(list: readonly T[], current: T): T {
  return list[(list.indexOf(current) + 1) % list.length] as T;
}

function seconds(value: number): string {
  return value < 0 ? '?' : `${value.toFixed(1)} s`;
}

export function VideoScreen() {
  const [sample, setSample] = useState(0);
  const [player, setPlayer] = useState(() => openPlayer(SAMPLES[0]!.uri));
  const [backdrop, setBackdrop] = useState(false);
  const [fit, setFit] = useState<VideoContentFit>('contain');
  const [small, setSmall] = useState(false);
  const [fullscreen, setFullscreen] = useState(false);
  const [focus, setFocus] = useState(0);
  const probing = useRef(false);
  const view = useRef<VideoView>(null);
  const report = usePlayerReport(player);
  const uri = SAMPLES[sample]!.uri;

  useEffect(() => {
    console.log(`[video-screen] source ${SAMPLES[sample]!.name}: ${uri}`);
  }, [sample, uri]);

  useEffect(() => {
    const subscription = player.addListener('statusChange', ({ status }) => {
      if (status !== 'readyToPlay' || !probing.current) return;
      probing.current = false;
      probeAtReady(player);
    });
    return () => subscription.remove();
  }, [player]);

  const switchSample = (step: number) => {
    const index = (sample + step + SAMPLES.length) % SAMPLES.length;
    setSample(index);
    player.replace({ uri: SAMPLES[index]!.uri });
    player.play();
  };

  const toggleFullscreen = () => {
    const entering = !fullscreen;
    setFullscreen(entering);
    const call = entering ? view.current?.enterFullscreen() : view.current?.exitFullscreen();
    Promise.resolve(call).catch((error: unknown) =>
      console.log(`[video-screen] fullscreen failed: ${String(error)}`),
    );
  };

  const run = (action: (typeof ACTIONS)[number]) => {
    console.log(`[video-screen] action ${action}`);
    switch (action) {
      case 'Backdrop behind the video':
        return setBackdrop((current) => !current);
      case 'Content fit':
        return setFit((current) => next(FITS, current));
      case 'Box size':
        return setSmall((current) => !current);
      case 'Play / pause':
        return player.playing ? player.pause() : player.play();
      case 'Seek +30 s':
        return player.seekBy(30);
      case 'Seek -10 s':
        return player.seekBy(-10);
      case 'Next audio track':
        return selectNextAudioTrack(player);
      case 'Next subtitle track, then off':
        return selectNextSubtitle(player);
      case 'Fullscreen':
        return toggleFullscreen();
      case 'Probe seek and audio track at READY':
        probing.current = true;
        player.pause();
        player.replace({ uri });
        return;
      case 'Swap to a new player': {
        const retired = player;
        retired.pause();
        setPlayer(openPlayer(uri));
        setTimeout(() => retired.release(), 1000);
        return;
      }
    }
  };

  useTVEventHandler((event) => {
    if (event.eventType === 'up') setFocus((current) => Math.max(0, current - 1));
    if (event.eventType === 'down')
      setFocus((current) => Math.min(ACTIONS.length - 1, current + 1));
    if (event.eventType === 'left') switchSample(-1);
    if (event.eventType === 'right') switchSample(1);
    if (event.eventType === 'select') run(fullscreen ? 'Fullscreen' : ACTIONS[focus]!);
    if (event.eventType === 'playPause') run('Play / pause');
  });

  const error = report.error ? ` (${report.error})` : '';
  const lines = [
    `Backdrop: ${backdrop ? 'purple, opaque' : 'none'} · Fit: ${fit} · Box: ${small ? 'square, lower' : 'large 16:9'}`,
    `Source ${sample + 1}/${SAMPLES.length}: ${SAMPLES[sample]!.name}`,
    `Status: ${report.status}${error} · ${report.playing ? 'playing' : 'not playing'}`,
    `Time: ${seconds(report.time)} of ${seconds(report.duration)} · buffered ${seconds(report.buffered)}`,
    `Video: ${describeVideo(report.video)}`,
    `Audio: ${report.audioTracks.length} tracks, current ${report.audioTrack?.label ?? 'none'} · Text: ${report.subtitleCount}, current ${report.subtitleTrack?.label ?? 'off'}`,
    `Last event: ${report.lastEvent}`,
  ];

  return (
    <View style={styles.screen}>
      <View style={styles.panel}>
        <Text style={styles.title}>expo-video</Text>
        <Text style={styles.hint}>Left / Right: source · Up / Down: action · OK: run it</Text>
        {lines.map((line) => (
          <Text key={line.slice(0, 12)} style={styles.line}>
            {line}
          </Text>
        ))}
        <View style={styles.menu}>
          {ACTIONS.map((action, index) => (
            <Text key={action} style={[styles.action, index === focus && styles.focused]}>
              {action}
            </Text>
          ))}
        </View>
      </View>
      <View style={[styles.box, small ? styles.small : styles.large, backdrop && styles.backdrop]}>
        <VideoView
          ref={view}
          player={player}
          style={StyleSheet.absoluteFill}
          contentFit={fit}
          nativeControls={false}
          onFirstFrameRender={() => console.log('[video-screen] onFirstFrameRender')}
          onFullscreenEnter={() => console.log('[video-screen] onFullscreenEnter')}
          onFullscreenExit={() => console.log('[video-screen] onFullscreenExit')}
        />
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  screen: { flex: 1 },
  panel: {
    position: 'absolute',
    left: 0,
    top: 0,
    bottom: 0,
    width: 680,
    backgroundColor: '#0b0f1a',
    paddingHorizontal: 40,
    paddingVertical: 40,
  },
  title: { color: '#ffffff', fontSize: 40, fontWeight: '700' },
  hint: { color: '#7c8db5', fontSize: 20, marginTop: 6, marginBottom: 16 },
  line: { color: '#e5e9f5', fontSize: 20, marginTop: 6 },
  menu: { marginTop: 20 },
  action: { color: '#7c8db5', fontSize: 22, paddingVertical: 5, paddingHorizontal: 12 },
  focused: { color: '#0b0f1a', backgroundColor: '#ffd23f' },
  box: { position: 'absolute', borderWidth: 4, borderColor: '#ffd23f' },
  large: { left: 720, top: 200, width: 1168, height: 664 },
  small: { left: 760, top: 460, width: 528, height: 528 },
  backdrop: { backgroundColor: '#5b2a86' },
});
