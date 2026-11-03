import { StyleSheet, Text, View, type ViewStyle } from 'react-native';
import { SvgGradients } from './svg-gradients.tsx';

interface Sample {
  readonly label: string;
  readonly style: ViewStyle;
}

const CSS_SAMPLES: readonly Sample[] = [
  {
    label: 'to bottom',
    style: { experimental_backgroundImage: 'linear-gradient(to bottom, #ff5252, #3d5afe)' },
  },
  {
    label: 'transparent stop',
    style: {
      backgroundColor: '#ffb300',
      experimental_backgroundImage: 'linear-gradient(90deg, #0b0f1a 10%, transparent 70%)',
    },
  },
  {
    label: '135deg, rounded',
    style: {
      borderRadius: 40,
      experimental_backgroundImage: 'linear-gradient(135deg, #F4B642, #E8743B)',
    },
  },
  {
    label: 'to top right',
    style: {
      experimental_backgroundImage: 'linear-gradient(to top right, #ff5252, #ffee58 30%, #3d5afe)',
    },
  },
  {
    label: 'hint 20%',
    style: { experimental_backgroundImage: 'linear-gradient(to right, #ff5252, 20%, #3d5afe)' },
  },
  {
    label: 'px stops',
    style: {
      experimental_backgroundImage: 'linear-gradient(to right, #000 0px, #fff 40px, #000 80px)',
    },
  },
  {
    label: 'radial circle',
    style: {
      backgroundColor: '#1a2236',
      experimental_backgroundImage: 'radial-gradient(circle, #ffb300, transparent 70%)',
    },
  },
  {
    label: 'radial 58% 46%',
    style: {
      backgroundColor: '#3d5afe',
      experimental_backgroundImage:
        'radial-gradient(58% 46% at 50% 50%, rgba(8, 8, 10, 0.9) 0%, rgba(8, 8, 10, 0.34) 58%, rgba(8, 8, 10, 0) 100%)',
    },
  },
  {
    label: 'ellipse at top left',
    style: {
      experimental_backgroundImage: 'radial-gradient(ellipse at top left, #00e676, #0b0f1a)',
    },
  },
  {
    label: 'two layers',
    style: {
      backgroundColor: '#ffb300',
      experimental_backgroundImage:
        'linear-gradient(0deg, #0b0f1a 4%, transparent 60%), linear-gradient(90deg, #0b0f1a 12%, transparent 68%)',
    },
  },
  {
    label: 'border, radius',
    style: {
      borderRadius: 32,
      borderWidth: 6,
      borderColor: '#ffffff',
      experimental_backgroundImage: 'linear-gradient(to bottom, #00e676, #3d5afe)',
    },
  },
  {
    label: 'dashed',
    style: { borderRadius: 32, borderWidth: 2, borderColor: '#a9b4cf', borderStyle: 'dashed' },
  },
  {
    label: 'dotted',
    style: { borderRadius: 32, borderWidth: 4, borderColor: '#ffb300', borderStyle: 'dotted' },
  },
];

/** `experimental_backgroundImage`, dashed and dotted borders, and react-native-svg gradients on Tizen. */
export function GradientScreen() {
  return (
    <View style={styles.screen}>
      <Text style={styles.title}>Gradients and patterned borders</Text>
      <View style={styles.grid}>
        {CSS_SAMPLES.map((sample) => (
          <View key={sample.label} style={styles.cell}>
            <View style={[styles.swatch, sample.style]} />
            <Text style={styles.label}>{sample.label}</Text>
          </View>
        ))}
      </View>
      <SvgGradients />
    </View>
  );
}

const styles = StyleSheet.create({
  screen: { flex: 1, backgroundColor: '#0b0f1a', paddingHorizontal: 96, paddingVertical: 48 },
  title: { color: '#ffffff', fontSize: 40, fontWeight: '700', marginBottom: 24 },
  grid: { flexDirection: 'row', flexWrap: 'wrap', gap: 24 },
  cell: { width: 220, alignItems: 'center' },
  swatch: { width: 220, height: 140 },
  label: { color: '#a9b4cf', fontSize: 20, marginTop: 8 },
});
