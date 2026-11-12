import { useState } from 'react';
import { useTVEventHandler } from 'react-native';
import { BoxScreen } from './box-screen.tsx';
import { ImageScreen } from './image-screen.tsx';
import { ModalScreen } from './modal-screen.tsx';
import { ScrollScreen } from './scroll-screen.tsx';
import { TextScreen } from './text-screen.tsx';
import { TransformScreen } from './transform-screen.tsx';

const SCREENS = [
  ScrollScreen,
  ImageScreen,
  BoxScreen,
  TransformScreen,
  TextScreen,
  ModalScreen,
] as const;

/** The rendering screen the number keys picked: 1 to 6 open one, 0 returns home (null). */
export function useRenderingScreen(): number | null {
  const [screen, setScreen] = useState<number | null>(null);
  useTVEventHandler((event) => {
    const digit = Number(event.eventType);
    if (!Number.isInteger(digit) || event.eventType.length !== 1) return;
    setScreen(digit >= 1 && digit <= SCREENS.length ? digit - 1 : null);
  });
  return screen;
}

export function RenderingScreen({ index }: { readonly index: number }) {
  const Screen = SCREENS[index] ?? ScrollScreen;
  return <Screen />;
}
