import { useState } from 'react';
import {
  Image,
  type ImageResizeMode,
  type ImageSourcePropType,
  StyleSheet,
  Text,
  View,
} from 'react-native';
import poster from './poster.png';
import { ScreenFrame, palette } from './screen-frame.tsx';

const PHOTO = { uri: 'https://picsum.photos/id/1015/600/400.jpg' };
const CHECKER = {
  uri: 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAgAAAAICAYAAADED76LAAAAHklEQVR42mP4HxDwHxkHLPiPghnooABdAF0DHRQAAGWdsYHKDlQnAAAAAElFTkSuQmCC',
};
const MISSING = { uri: 'https://picsum.photos/id/1015/404-not-found.jpg' };

interface Sample {
  readonly label: string;
  readonly source: ImageSourcePropType;
  readonly resizeMode: ImageResizeMode;
  readonly rounded?: boolean;
}

const SAMPLES: readonly Sample[] = [
  { label: 'cover', source: PHOTO, resizeMode: 'cover' },
  { label: 'contain', source: PHOTO, resizeMode: 'contain' },
  { label: 'stretch', source: PHOTO, resizeMode: 'stretch' },
  { label: 'center', source: PHOTO, resizeMode: 'center' },
  { label: 'bundled, radius', source: poster, resizeMode: 'cover', rounded: true },
  { label: 'data: repeat', source: CHECKER, resizeMode: 'repeat' },
  { label: 'data: stretch', source: CHECKER, resizeMode: 'stretch' },
  { label: '404', source: MISSING, resizeMode: 'cover' },
];

function ImageSample({ sample }: { readonly sample: Sample }) {
  const [status, setStatus] = useState('loading');
  return (
    <View style={styles.sample}>
      <Image
        source={sample.source}
        resizeMode={sample.resizeMode}
        style={[styles.image, sample.rounded && styles.rounded]}
        onLoad={(event) => {
          const { width, height } = event.nativeEvent.source;
          setStatus(`loaded ${width}x${height}`);
        }}
        onError={(event) => setStatus(`error: ${event.nativeEvent.error}`)}
      />
      <Text style={styles.label}>{sample.label}</Text>
      <Text numberOfLines={1} style={styles.status}>
        {status}
      </Text>
    </View>
  );
}

export function ImageScreen() {
  return (
    <ScreenFrame
      title="Image"
      hint="Network JPEG, bundled PNG, data: URI and a 404, with every resizeMode"
    >
      <View style={styles.grid}>
        {SAMPLES.map((sample) => (
          <ImageSample key={sample.label} sample={sample} />
        ))}
      </View>
    </ScreenFrame>
  );
}

const styles = StyleSheet.create({
  grid: { flexDirection: 'row', flexWrap: 'wrap', gap: 32 },
  sample: { width: 380 },
  image: {
    width: 380,
    height: 240,
    backgroundColor: '#26314d',
    borderWidth: 2,
    borderColor: '#4a5a80',
  },
  rounded: { borderRadius: 32, borderWidth: 0 },
  label: { color: palette.text, fontSize: 26, marginTop: 8 },
  status: { color: palette.amber, fontSize: 22 },
});
