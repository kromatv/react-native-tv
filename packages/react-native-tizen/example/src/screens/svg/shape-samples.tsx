import type { ReactNode } from 'react';
import { StyleSheet, Text, View } from 'react-native';
import Svg, { Circle, Ellipse, G, Line, Path, Polygon, Polyline, Rect } from 'react-native-svg';

const STROKE = '#ffffff';

function Sample({ label, children }: { readonly label: string; readonly children: ReactNode }) {
  return (
    <View style={styles.sample}>
      {children}
      <Text style={styles.label}>{label}</Text>
    </View>
  );
}

/** One `<Svg>` per element and option, each in a 120 x 120 frame. */
export function ShapeSamples() {
  return (
    <View style={styles.row}>
      <Sample label="circle">
        <Svg width={120} height={120} viewBox="0 0 100 100" style={styles.frame}>
          <Circle cx={50} cy={50} r={40} fill="#3d5afe" stroke={STROKE} strokeWidth={4} />
        </Svg>
      </Sample>
      <Sample label="ellipse">
        <Svg width={120} height={120} viewBox="0 0 100 100" style={styles.frame}>
          <Ellipse cx="50%" cy="50%" rx={44} ry={24} fill="#ff5c5c" />
        </Svg>
      </Sample>
      <Sample label="rect rx ry">
        <Svg width={120} height={120} viewBox="0 0 100 100" style={styles.frame}>
          <Rect
            x={10}
            y={20}
            width={80}
            height={60}
            rx={16}
            fill="#4cd964"
            stroke={STROKE}
            strokeWidth={3}
          />
        </Svg>
      </Sample>
      <Sample label="line caps">
        <Svg width={120} height={120} viewBox="0 0 100 100" style={styles.frame}>
          <Line
            x1={20}
            y1={25}
            x2={80}
            y2={25}
            stroke={STROKE}
            strokeWidth={10}
            strokeLinecap="butt"
          />
          <Line
            x1={20}
            y1={50}
            x2={80}
            y2={50}
            stroke={STROKE}
            strokeWidth={10}
            strokeLinecap="round"
          />
          <Line
            x1={20}
            y1={75}
            x2={80}
            y2={75}
            stroke={STROKE}
            strokeWidth={10}
            strokeLinecap="square"
          />
        </Svg>
      </Sample>
      <Sample label="polyline joins">
        <Svg width={120} height={120} viewBox="0 0 100 100" style={styles.frame}>
          <Polyline
            points="15,40 35,15 55,40"
            fill="none"
            stroke={STROKE}
            strokeWidth={8}
            strokeLinejoin="miter"
          />
          <Polyline
            points="45,85 65,60 85,85"
            fill="none"
            stroke="#ffd60a"
            strokeWidth={8}
            strokeLinejoin="round"
          />
        </Svg>
      </Sample>
      <Sample label="polygon">
        <Svg width={120} height={120} viewBox="0 0 100 100" style={styles.frame}>
          <Polygon
            points="50,8 61,38 94,38 67,58 78,90 50,70 22,90 33,58 6,38 39,38"
            fill="#ffd60a"
          />
        </Svg>
      </Sample>
      <Sample label="g opacity">
        <Svg width={120} height={120} viewBox="0 0 100 100" style={styles.frame}>
          <G opacity={0.5} fill="#ff5c5c">
            <Rect x={10} y={10} width={50} height={50} />
            <Rect x={40} y={40} width={50} height={50} fill="#3d5afe" />
          </G>
        </Svg>
      </Sample>
      <Sample label="transform">
        <Svg width={120} height={120} viewBox="0 0 100 100" style={styles.frame}>
          <G transform="rotate(30 50 50)">
            <Rect
              x={25}
              y={25}
              width={50}
              height={50}
              fill="#4cd964"
              fillOpacity={0.5}
              stroke={STROKE}
              strokeOpacity={0.6}
              strokeWidth={6}
            />
          </G>
        </Svg>
      </Sample>
      <Sample label="arcs, curves">
        <Svg width={120} height={120} viewBox="0 0 100 100" style={styles.frame}>
          <Path d="M10 80 Q 30 10 50 80 T 90 80" fill="none" stroke="#4cd964" strokeWidth={4} />
          <Path d="M20 30 a20 12 30 1 1 40 0" fill="none" stroke={STROKE} strokeWidth={4} />
        </Svg>
      </Sample>
      <Sample label="slice, none">
        <View style={styles.pair}>
          <Svg
            width={56}
            height={120}
            viewBox="0 0 100 100"
            preserveAspectRatio="xMidYMid slice"
            style={styles.frame}
          >
            <Circle cx={50} cy={50} r={45} fill="#3d5afe" />
          </Svg>
          <Svg
            width={56}
            height={120}
            viewBox="0 0 100 100"
            preserveAspectRatio="none"
            style={styles.frame}
          >
            <Circle cx={50} cy={50} r={45} fill="#ff5c5c" />
          </Svg>
        </View>
      </Sample>
    </View>
  );
}

const styles = StyleSheet.create({
  row: { flexDirection: 'row', gap: 24 },
  sample: { alignItems: 'center' },
  frame: { backgroundColor: '#1a2236' },
  pair: { flexDirection: 'row', gap: 8 },
  label: { color: '#a9b4cf', fontSize: 18, marginTop: 8 },
});
