import { StyleSheet, Text, View } from 'react-native';
import { ScreenFrame, palette } from './screen-frame.tsx';

const LONG =
  "Une soirée au cinéma : la salle s'éteint, le projecteur s'allume, et chaque spectateur oublie la ville dehors pendant deux heures.";

export function TextScreen() {
  return (
    <ScreenFrame
      title="Text"
      hint="numberOfLines, ellipsizeMode, lineHeight, textAlign, decorations, textTransform"
    >
      <View style={styles.columns}>
        <View style={styles.column}>
          <Text numberOfLines={1} style={styles.boxed}>{`tail: ${LONG}`}</Text>
          <Text numberOfLines={1} ellipsizeMode="head" style={styles.boxed}>{`head: ${LONG}`}</Text>
          <Text
            numberOfLines={1}
            ellipsizeMode="middle"
            style={styles.boxed}
          >{`middle: ${LONG}`}</Text>
          <Text numberOfLines={1} ellipsizeMode="clip" style={styles.boxed}>{`clip: ${LONG}`}</Text>
          <Text numberOfLines={2} style={styles.boxed}>{`two lines: ${LONG}`}</Text>
          <Text style={[styles.boxed, styles.tall]}>{`lineHeight 48: ${LONG}`}</Text>
        </View>
        <View style={styles.column}>
          <Text style={[styles.boxed, styles.left]}>textAlign left</Text>
          <Text style={[styles.boxed, styles.center]}>textAlign center</Text>
          <Text style={[styles.boxed, styles.right]}>textAlign right</Text>
          <Text style={styles.boxed}>
            <Text style={styles.underline}>underline</Text>
            {' · '}
            <Text style={styles.strike}>line-through</Text>
            {' · '}
            <Text style={styles.both}>both</Text>
            {' · '}
            <Text style={styles.marked}>background</Text>
          </Text>
          <Text style={[styles.boxed, styles.upper]}>{"uppercase : élève à l'école, œuvre"}</Text>
          <Text style={[styles.boxed, styles.lower]}>{"LOWERCASE : ÉLÈVE À L'ÉCOLE"}</Text>
          <Text style={[styles.boxed, styles.capitalize]}>{"capitalize : élève à l'école"}</Text>
          <Text style={[styles.boxed, styles.faded]}>
            Opacity 0.4 on <Text style={styles.bold}>bold</Text> and{' '}
            <Text style={styles.italic}>italic</Text>
          </Text>
        </View>
      </View>
    </ScreenFrame>
  );
}

const styles = StyleSheet.create({
  columns: { flexDirection: 'row', gap: 64 },
  column: { flex: 1, gap: 18 },
  boxed: { color: palette.text, fontSize: 28, backgroundColor: palette.card, padding: 10 },
  tall: { lineHeight: 48 },
  left: { textAlign: 'left' },
  center: { textAlign: 'center' },
  right: { textAlign: 'right' },
  underline: { textDecorationLine: 'underline', textDecorationColor: palette.amber },
  strike: { textDecorationLine: 'line-through' },
  both: { textDecorationLine: 'underline line-through' },
  marked: { backgroundColor: '#c62828' },
  upper: { textTransform: 'uppercase' },
  lower: { textTransform: 'lowercase' },
  capitalize: { textTransform: 'capitalize' },
  faded: { opacity: 0.4 },
  bold: { fontWeight: '700' },
  italic: { fontStyle: 'italic' },
});
