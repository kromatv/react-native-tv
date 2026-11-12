import type { ReactNode } from 'react';
import { StyleSheet, Text as Label, View } from 'react-native';
import Svg, {
  Circle,
  ClipPath,
  Defs,
  G,
  LinearGradient,
  Mask,
  Path,
  Pattern,
  RadialGradient,
  Rect,
  Stop,
  Symbol,
  TSpan,
  Text,
  Use,
} from 'react-native-svg';

const STAR = 'M50 8 L61 38 L94 38 L67 58 L78 90 L50 70 L22 90 L33 58 L6 38 L39 38 Z';
const PENTAGRAM = 'M50 6 L79 94 L4 39 L96 39 L21 94 Z';

function Sample({ label, children }: { readonly label: string; readonly children: ReactNode }) {
  return (
    <View style={styles.sample}>
      <Svg width={120} height={120} viewBox="0 0 100 100" style={styles.frame}>
        {children}
      </Svg>
      <Label style={styles.label}>{label}</Label>
    </View>
  );
}

/** What Skia added to the port: dashes, fill rules, clips, masks, `Use`, `Symbol`, text and patterns. */
export function SkiaSamples() {
  return (
    <View style={styles.row}>
      <Sample label="dashes">
        <Circle
          cx={50}
          cy={50}
          r={38}
          fill="none"
          stroke="#ffffff"
          strokeWidth={6}
          strokeDasharray="12 8"
        />
        <Path
          d="M20 50 H80"
          stroke="#ffd60a"
          strokeWidth={6}
          strokeLinecap="round"
          strokeDasharray={[1, 11]}
        />
      </Sample>
      <Sample label="evenodd, nonzero">
        <G transform="scale(0.5)">
          <Path d={PENTAGRAM} fill="#ff5c5c" fillRule="evenodd" />
        </G>
        <G transform="translate(50 50) scale(0.5)">
          <Path d={PENTAGRAM} fill="#4cd964" fillRule="nonzero" />
        </G>
      </Sample>
      <Sample label="clip path">
        <Defs>
          <ClipPath id="star">
            <Path d={STAR} />
          </ClipPath>
          <LinearGradient id="stripes" x1="0" y1="0" x2="1" y2="1">
            <Stop offset="0" stopColor="#3d5afe" />
            <Stop offset="1" stopColor="#ff5c5c" />
          </LinearGradient>
        </Defs>
        <G clipPath="url(#star)">
          <Rect x={0} y={0} width={100} height={100} fill="url(#stripes)" />
          <Circle cx={50} cy={50} r={18} fill="#ffffff" />
        </G>
      </Sample>
      <Sample label="clip rules">
        <Defs>
          <ClipPath id="rings">
            <Circle cx={38} cy={50} r={28} />
            <Circle cx={62} cy={50} r={28} />
          </ClipPath>
        </Defs>
        <Rect x={0} y={0} width={100} height={50} fill="#ffd60a" clipPath="url(#rings)" />
        <Rect
          x={0}
          y={50}
          width={100}
          height={50}
          fill="#4cd964"
          clipPath="url(#rings)"
          clipRule="nonzero"
        />
      </Sample>
      <Sample label="mask">
        <Defs>
          <LinearGradient id="fade" x1="0" y1="0" x2="1" y2="0">
            <Stop offset="0" stopColor="#ffffff" />
            <Stop offset="1" stopColor="#000000" />
          </LinearGradient>
          <Mask id="fading">
            <Rect x={0} y={0} width={100} height={100} fill="url(#fade)" />
          </Mask>
        </Defs>
        <G mask="url(#fading)">
          <Rect x={10} y={10} width={80} height={80} rx={12} fill="#3d5afe" />
          <Circle cx={50} cy={50} r={24} fill="#ffd60a" />
        </G>
      </Sample>
      <Sample label="use, symbol">
        <Defs>
          <G id="leaf">
            <Path d="M0 20 Q10 0 20 20 Q10 40 0 20 Z" />
          </G>
          <Symbol id="dot" viewBox="0 0 10 10">
            <Circle cx={5} cy={5} r={5} />
          </Symbol>
        </Defs>
        <Use href="#leaf" x={10} y={8} fill="#4cd964" />
        <Use href="#leaf" x={40} y={8} fill="#ffd60a" />
        <Use href="#leaf" x={70} y={8} fill="#ff5c5c" opacity={0.5} />
        <Use href="#dot" x={10} y={60} width={30} height={30} fill="#3d5afe" />
        <Use href="#dot" x={50} y={55} width={40} height={40} fill="#ffffff" />
      </Sample>
      <Sample label="text">
        <Text x={50} y={30} fontSize={22} fontWeight="bold" fill="#ffffff" textAnchor="middle">
          Skia
        </Text>
        <Text x={8} y={60} fontSize={16} fill="#a9b4cf">
          a<TSpan fill="#ffd60a">b</TSpan>
          <TSpan fill="#ff5c5c" dy={6}>
            c
          </TSpan>
        </Text>
        <Text
          x={92}
          y={90}
          fontSize={26}
          fill="none"
          stroke="#4cd964"
          strokeWidth={1}
          textAnchor="end"
        >
          end
        </Text>
      </Sample>
      <Sample label="pattern">
        <Defs>
          <Pattern id="dots" x={0} y={0} width={20} height={20} patternUnits="userSpaceOnUse">
            <Circle cx={10} cy={10} r={6} fill="#ffd60a" />
          </Pattern>
        </Defs>
        <Rect
          x={10}
          y={10}
          width={80}
          height={80}
          rx={12}
          fill="url(#dots)"
          stroke="#ffffff"
          strokeWidth={2}
        />
      </Sample>
      <Sample label="focus, opacity">
        <Defs>
          <RadialGradient id="ball" cx="50%" cy="50%" r="50%" fx="30%" fy="30%">
            <Stop offset="0" stopColor="#ffffff" />
            <Stop offset="1" stopColor="#3d5afe" />
          </RadialGradient>
        </Defs>
        <Circle cx={36} cy={36} r={30} fill="url(#ball)" />
        <Rect
          x={44}
          y={44}
          width={46}
          height={46}
          fill="#ff5c5c"
          stroke="#ffffff"
          strokeWidth={8}
          opacity={0.5}
        />
      </Sample>
    </View>
  );
}

const styles = StyleSheet.create({
  row: { flexDirection: 'row', gap: 24, marginTop: 20 },
  sample: { alignItems: 'center' },
  frame: { backgroundColor: '#1a2236' },
  label: { color: '#a9b4cf', fontSize: 18, marginTop: 8 },
});
