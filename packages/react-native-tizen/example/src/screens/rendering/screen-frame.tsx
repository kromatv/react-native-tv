import type { ReactNode } from 'react';
import { StyleSheet, Text, View } from 'react-native';

interface ScreenFrameProps {
  readonly title: string;
  readonly hint: string;
  readonly children: ReactNode;
}

export function ScreenFrame({ title, hint, children }: ScreenFrameProps) {
  return (
    <View style={styles.screen}>
      <Text style={styles.title}>{title}</Text>
      <Text style={styles.hint}>{hint}</Text>
      <View style={styles.body}>{children}</View>
    </View>
  );
}

export const palette = {
  background: '#0b0f1a',
  card: '#1a2236',
  accent: '#3d5afe',
  amber: '#ffb300',
  text: '#ffffff',
  muted: '#a9b4cf',
} as const;

const styles = StyleSheet.create({
  screen: {
    flex: 1,
    backgroundColor: palette.background,
    paddingHorizontal: 96,
    paddingVertical: 56,
  },
  title: { color: palette.text, fontSize: 48, fontWeight: '700' },
  hint: { color: palette.muted, fontSize: 24, marginTop: 8 },
  body: { flex: 1, marginTop: 32 },
});
