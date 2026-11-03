import type { AudioTrack, SubtitleTrack, VideoPlayer, VideoTrack } from 'expo-video';
import { useEffect, useState } from 'react';

export interface PlayerReport {
  readonly status: string;
  readonly error: string | null;
  readonly playing: boolean;
  readonly time: number;
  readonly duration: number;
  readonly buffered: number;
  readonly video: VideoTrack | null;
  readonly audioTracks: readonly AudioTrack[];
  readonly audioTrack: AudioTrack | null;
  readonly subtitleCount: number;
  readonly subtitleTrack: SubtitleTrack | null;
  readonly lastEvent: string;
}

const EMPTY: PlayerReport = {
  status: 'idle',
  error: null,
  playing: false,
  time: 0,
  duration: 0,
  buffered: -1,
  video: null,
  audioTracks: [],
  audioTrack: null,
  subtitleCount: 0,
  subtitleTrack: null,
  lastEvent: 'none',
};

function log(label: string, payload: unknown): void {
  console.log(`[video-screen] ${label} ${JSON.stringify(payload)}`);
}

export function usePlayerReport(player: VideoPlayer): PlayerReport {
  const [report, setReport] = useState<PlayerReport>(EMPTY);

  useEffect(() => {
    const update = (patch: Partial<PlayerReport>) =>
      setReport((current) => ({ ...current, ...patch }));
    update({ ...EMPTY, status: player.status, playing: player.playing });
    const subscriptions = [
      player.addListener('statusChange', (event) => {
        log('statusChange', event);
        update({
          status: event.status,
          error: event.error?.message ?? null,
          lastEvent: `status ${event.status}`,
        });
      }),
      player.addListener('playingChange', (event) => {
        log('playingChange', event);
        update({ playing: event.isPlaying, lastEvent: `playing ${event.isPlaying}` });
      }),
      player.addListener('timeUpdate', (event) => {
        update({
          time: event.currentTime,
          buffered: event.bufferedPosition,
          duration: player.duration,
        });
      }),
      player.addListener('sourceLoad', (event) => {
        log('sourceLoad', event);
        update({
          duration: event.duration,
          video: event.availableVideoTracks[0] ?? null,
          audioTracks: event.availableAudioTracks,
          audioTrack: player.audioTrack,
          subtitleCount: event.availableSubtitleTracks.length,
          lastEvent: 'sourceLoad',
        });
      }),
      player.addListener('videoTrackChange', (event) => {
        log('videoTrackChange', event);
        update({ video: event.videoTrack, lastEvent: 'videoTrackChange' });
      }),
      player.addListener('audioTrackChange', (event) => {
        log('audioTrackChange', event);
        update({ audioTrack: event.audioTrack, lastEvent: 'audioTrackChange' });
      }),
      player.addListener('subtitleTrackChange', (event) => {
        log('subtitleTrackChange', event);
        update({ subtitleTrack: event.subtitleTrack, lastEvent: 'subtitleTrackChange' });
      }),
      player.addListener('playToEnd', () => {
        log('playToEnd', {});
        update({ lastEvent: 'playToEnd' });
      }),
      player.addListener('sourceChange', (event) => log('sourceChange', event)),
    ];
    const ticker = setInterval(() => {
      log('tick', {
        status: player.status,
        playing: player.playing,
        currentTime: Number(player.currentTime.toFixed(2)),
        duration: player.duration,
        bufferedPosition: player.bufferedPosition,
      });
    }, 2000);
    return () => {
      clearInterval(ticker);
      for (const subscription of subscriptions) subscription.remove();
    };
  }, [player]);

  return report;
}
