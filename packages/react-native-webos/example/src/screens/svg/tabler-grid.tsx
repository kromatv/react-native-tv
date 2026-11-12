import {
  IconArrowLeft,
  IconBell,
  IconCheck,
  IconChevronRight,
  IconDeviceTv,
  IconDownload,
  IconHeart,
  IconHome,
  IconMinus,
  IconMovie,
  IconPlayerPlay,
  IconPlus,
  IconSearch,
  IconSettings,
  IconStar,
  IconUser,
  type Icon,
} from '@tabler/icons-react-native';
import { StyleSheet, Text, View } from 'react-native';

const ICONS: readonly (readonly [string, Icon])[] = [
  ['home', IconHome],
  ['search', IconSearch],
  ['settings', IconSettings],
  ['player-play', IconPlayerPlay],
  ['heart', IconHeart],
  ['user', IconUser],
  ['bell', IconBell],
  ['star', IconStar],
  ['check', IconCheck],
  ['plus', IconPlus],
  ['minus', IconMinus],
  ['chevron-right', IconChevronRight],
  ['arrow-left', IconArrowLeft],
  ['download', IconDownload],
  ['movie', IconMovie],
  ['device-tv', IconDeviceTv],
];

const SIZES = [24, 48, 96] as const;
const STROKES = [1, 2, 3] as const;
const COLORS = ['#ff5c5c', '#4cd964', '#3d5afe'] as const;

/** Tabler's outline icons as an app draws them: stroked paths, round caps and joins. */
export function TablerGrid() {
  return (
    <View>
      <View style={styles.grid}>
        {ICONS.map(([name, Glyph]) => (
          <View key={name} style={styles.cell}>
            <Glyph size={64} color="#ffffff" strokeWidth={2} />
            <Text style={styles.label}>{name}</Text>
          </View>
        ))}
      </View>
      <View style={styles.variants}>
        {SIZES.map((size) => (
          <IconHeart key={`size-${size}`} size={size} color="#ffffff" />
        ))}
        {STROKES.map((stroke) => (
          <IconStar key={`stroke-${stroke}`} size={72} color="#ffd60a" strokeWidth={stroke} />
        ))}
        {COLORS.map((color) => (
          <IconSettings key={color} size={72} color={color} />
        ))}
        <View style={styles.currentColor}>
          <IconBell size={72} />
        </View>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  grid: { flexDirection: 'row', flexWrap: 'wrap', width: 1200 },
  cell: { width: 150, height: 120, alignItems: 'center', justifyContent: 'center' },
  label: { color: '#a9b4cf', fontSize: 18, marginTop: 8 },
  variants: { flexDirection: 'row', alignItems: 'center', gap: 28, marginTop: 24 },
  currentColor: { backgroundColor: '#ffffff', borderRadius: 12, padding: 8 },
});
