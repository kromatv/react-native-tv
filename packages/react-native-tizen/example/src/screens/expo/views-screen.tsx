import { BlurView } from 'expo-blur';
import { Image, type ImageContentFit } from 'expo-image';
import { useState } from 'react';
import { StyleSheet, Text, View } from 'react-native';

const PHOTO = 'https://picsum.photos/id/237/600/400';
const FITS: readonly ImageContentFit[] = ['cover', 'contain', 'fill'];

function FittedImage({ fit }: { readonly fit: ImageContentFit }) {
  const [status, setStatus] = useState('loading');
  return (
    <View style={styles.cell}>
      <Image
        source={{ uri: PHOTO }}
        contentFit={fit}
        transition={300}
        style={styles.image}
        onLoad={(event) => {
          console.log(
            `[expo:views] PASS onLoad ${fit}: ${event.source.width}x${event.source.height} (${event.cacheType})`,
          );
          setStatus(`${event.source.width}x${event.source.height}, ${event.cacheType}`);
        }}
        onError={(event) => {
          console.log(`[expo:views] FAIL onError ${fit}: ${event.error}`);
          setStatus(event.error);
        }}
      />
      <Text style={styles.caption}>{`${fit}: ${status}`}</Text>
    </View>
  );
}

export function ViewsScreen() {
  return (
    <View style={styles.column}>
      <View style={styles.row}>
        {FITS.map((fit) => (
          <FittedImage key={fit} fit={fit} />
        ))}
      </View>
      <View style={styles.row}>
        <View style={styles.backdrop}>
          <Image source={{ uri: PHOTO }} contentFit="cover" style={StyleSheet.absoluteFill} />
          <BlurView tint="dark" intensity={80} style={styles.glass}>
            <Text style={styles.glassText}>BlurView tint dark, intensity 80</Text>
          </BlurView>
        </View>
        <View style={styles.backdrop}>
          <Image source={{ uri: PHOTO }} contentFit="cover" style={StyleSheet.absoluteFill} />
          <BlurView tint="light" intensity={60} style={styles.glass}>
            <Text style={[styles.glassText, styles.dark]}>BlurView tint light, intensity 60</Text>
          </BlurView>
        </View>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  column: { gap: 40 },
  row: { flexDirection: 'row', gap: 32 },
  cell: { gap: 12 },
  image: { width: 520, height: 300, backgroundColor: '#1a2236' },
  caption: { color: '#a9b4cf', fontSize: 22 },
  backdrop: { width: 800, height: 300, overflow: 'hidden', borderRadius: 16 },
  glass: {
    position: 'absolute',
    left: 40,
    right: 40,
    bottom: 40,
    height: 120,
    justifyContent: 'center',
    paddingHorizontal: 32,
  },
  glassText: { color: '#ffffff', fontSize: 28, fontWeight: '600' },
  dark: { color: '#0b0f1a' },
});
