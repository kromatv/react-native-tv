import { StyleSheet, Text, View } from 'react-native';
import Svg, { Circle, Defs, LinearGradient, RadialGradient, Rect, Stop } from 'react-native-svg';

/** react-native-svg's `LinearGradient` and `RadialGradient` as `url(#id)` fills and strokes. */
export function SvgGradients() {
  return (
    <View style={styles.row}>
      <View style={styles.cell}>
        <Svg width={220} height={140}>
          <Defs>
            <LinearGradient id="box" x1="0" y1="0" x2="1" y2="1">
              <Stop offset="0" stopColor="#ff5252" />
              <Stop offset="1" stopColor="#3d5afe" />
            </LinearGradient>
          </Defs>
          <Rect x="0" y="0" width="220" height="140" rx="24" fill="url(#box)" />
        </Svg>
        <Text style={styles.label}>linear, bounding box</Text>
      </View>
      <View style={styles.cell}>
        <Svg width={220} height={140}>
          <Defs>
            <LinearGradient id="ramp" x1="0" y1="0" x2="0" y2="140" gradientUnits="userSpaceOnUse">
              <Stop offset={0.3} stopColor="#ffb300" stopOpacity={1} />
              <Stop offset={1} stopColor="#ffb300" stopOpacity={0} />
            </LinearGradient>
          </Defs>
          <Rect x="0" y="0" width="100%" height="140" fill="url(#ramp)" />
        </Svg>
        <Text style={styles.label}>fade, user space</Text>
      </View>
      <View style={styles.cell}>
        <Svg width={220} height={140}>
          <Defs>
            <RadialGradient id="glow" cx="50%" cy="50%" r="50%">
              <Stop offset="0" stopColor="#00e676" />
              <Stop offset="1" stopColor="#0b0f1a" />
            </RadialGradient>
          </Defs>
          <Circle cx="110" cy="70" r="66" fill="url(#glow)" />
        </Svg>
        <Text style={styles.label}>radial</Text>
      </View>
      <View style={styles.cell}>
        <Svg width={220} height={140}>
          <Defs>
            <LinearGradient id="ring" x1="0" y1="0" x2="1" y2="0">
              <Stop offset="0" stopColor="#ffee58" />
              <Stop offset="1" stopColor="#ff5252" />
            </LinearGradient>
          </Defs>
          <Circle cx="110" cy="70" r="56" fill="none" stroke="url(#ring)" strokeWidth={14} />
        </Svg>
        <Text style={styles.label}>stroke</Text>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  row: { flexDirection: 'row', gap: 24, marginTop: 32 },
  cell: { width: 220, alignItems: 'center' },
  label: { color: '#a9b4cf', fontSize: 20, marginTop: 8 },
});
