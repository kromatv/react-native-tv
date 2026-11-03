export interface VideoSample {
  readonly name: string;
  readonly uri: string;
}

export const SAMPLES: readonly VideoSample[] = [
  {
    name: 'H.264 MP4, 1080p (Jellyfin)',
    uri: 'https://repo.jellyfin.org/test-videos/SDR/AVC/Test%20Jellyfin%201080p%20AVC%203M.mp4',
  },
  {
    name: 'HLS, MPEG-TS segments (Mux, Big Buck Bunny)',
    uri: 'https://test-streams.mux.dev/x36xhzz/x36xhzz.m3u8',
  },
  {
    name: 'HLS, fMP4 with alternate audio (Apple bipbop)',
    uri: 'https://devstreaming-cdn.apple.com/videos/streaming/examples/img_bipbop_adv_example_fmp4/master.m3u8',
  },
  {
    name: 'HEVC MKV, 8-bit (jellyfish)',
    uri: 'https://repo.jellyfin.org/archive/jellyfish/media/jellyfish-3-mbps-hd-hevc.mkv',
  },
  {
    name: 'HEVC MKV, Main10 (jellyfish)',
    uri: 'https://repo.jellyfin.org/archive/jellyfish/media/jellyfish-10-mbps-hd-hevc-10bit.mkv',
  },
  {
    name: 'HEVC HDR10 MP4, 1080p (Jellyfin)',
    uri: 'https://repo.jellyfin.org/test-videos/HDR/HDR10/HEVC/Test%20Jellyfin%201080p%20HEVC%20HDR10%2010M.mp4',
  },
  {
    name: 'H.264 MKV, two audio tracks (Matroska test5)',
    uri: 'https://raw.githubusercontent.com/ietf-wg-cellar/matroska-test-files/master/test_files/test5.mkv',
  },
];
