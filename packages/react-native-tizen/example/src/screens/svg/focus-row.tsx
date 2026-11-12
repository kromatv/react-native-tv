import { IconBell, IconHeart, IconHome, IconSearch, type Icon } from '@tabler/icons-react-native';
import { useState } from 'react';
import { StyleSheet, Text, View, useTVEventHandler } from 'react-native';
import Svg, { Circle, Path } from 'react-native-svg';

const ICONS: readonly (readonly [string, Icon])[] = [
  ['home', IconHome],
  ['search', IconSearch],
  ['heart', IconHeart],
  ['bell', IconBell],
];

/**
 * What a TV screen does to icons: Left and Right move the focus, which recolors
 * and resizes them, and Enter adds or removes a shape inside one `<Svg>`.
 */
export function FocusRow() {
  const [focused, setFocused] = useState(0);
  const [withDot, setWithDot] = useState(true);
  useTVEventHandler((event) => {
    if (event.eventType === 'right') setFocused((index) => Math.min(ICONS.length - 1, index + 1));
    if (event.eventType === 'left') setFocused((index) => Math.max(0, index - 1));
    if (event.eventType === 'select') setWithDot((shown) => !shown);
  });
  return (
    <View style={styles.row}>
      {ICONS.map(([name, Glyph], index) => (
        <Glyph
          key={name}
          size={index === focused ? 72 : 48}
          color={index === focused ? '#3d5afe' : '#7c8db5'}
        />
      ))}
      <Svg width={72} height={72} viewBox="0 0 24 24">
        <Path d="M4 12h16M12 4v16" stroke="#ffffff" strokeWidth={2} strokeLinecap="round" />
        {withDot ? <Circle cx={18} cy={6} r={3} fill="#ff5c5c" /> : null}
      </Svg>
      <Text style={styles.hint}>{`focus ${focused + 1}, dot ${withDot ? 'on' : 'off'}`}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  row: { flexDirection: 'row', alignItems: 'center', gap: 32, height: 96 },
  hint: { color: '#a9b4cf', fontSize: 22 },
});
