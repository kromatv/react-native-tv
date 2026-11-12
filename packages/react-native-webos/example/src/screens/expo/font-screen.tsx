import { getLoadedFonts, useFonts } from 'expo-font';
import { StyleSheet, Text, View } from 'react-native';

const FONTS = {
  Pacifico: 'https://raw.githubusercontent.com/google/fonts/main/ofl/pacifico/Pacifico-Regular.ttf',
  Montserrat:
    'https://raw.githubusercontent.com/google/fonts/main/ofl/montserrat/Montserrat%5Bwght%5D.ttf',
};

const WEIGHTS = ['100', '300', '400', '600', '700', '900'] as const;

function statusOf(loaded: boolean, error: Error | null): string {
  if (error) return `FAIL ${error.message}`;
  if (loaded) return `PASS loaded: ${getLoadedFonts().join(', ')}`;
  return 'Loading…';
}

export function FontScreen() {
  const [loaded, error] = useFonts(FONTS);
  const status = statusOf(loaded, error);
  if (loaded || error) console.log(`[expo:fonts] ${status}`);
  return (
    <View style={styles.column}>
      <Text style={styles.status}>{status}</Text>
      <Text style={styles.sample}>The system font, for comparison</Text>
      {loaded ? (
        <Text style={[styles.sample, styles.pacifico]}>Pacifico, loaded at run time</Text>
      ) : null}
      {loaded
        ? WEIGHTS.map((fontWeight) => (
            <Text key={fontWeight} style={[styles.variable, { fontWeight }]}>
              {`Montserrat, one variable file, at ${fontWeight}`}
            </Text>
          ))
        : null}
    </View>
  );
}

const styles = StyleSheet.create({
  column: { gap: 24 },
  status: { color: '#a9b4cf', fontSize: 26 },
  sample: { color: '#ffffff', fontSize: 64 },
  pacifico: { fontFamily: 'Pacifico' },
  variable: { color: '#ffffff', fontSize: 44, fontFamily: 'Montserrat' },
});
