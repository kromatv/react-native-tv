import { useState } from 'react';
import { useTVEventHandler } from 'react-native';
import { BlurScreen } from './blur-screen.tsx';
import { MaskScreen } from './mask-screen.tsx';
import { RailBlurScreen } from './rail-blur-screen.tsx';
import { TypeScreen } from './type-screen.tsx';

function BlurredRail() {
  return <RailBlurScreen blurred />;
}

function TintedRail() {
  return <RailBlurScreen blurred={false} />;
}

function BlurBesideRail() {
  return <RailBlurScreen blurred under={false} />;
}

const SCREENS = [
  BlurScreen,
  BlurredRail,
  TintedRail,
  MaskScreen,
  BlurBesideRail,
  TypeScreen,
] as const;

/** The second rendering pass's screens: 1 to 6 pick one. */
export function Rendering2Screens() {
  const [index, setIndex] = useState(0);
  useTVEventHandler((event) => {
    const digit = Number(event.eventType);
    if (!Number.isInteger(digit) || event.eventType.length !== 1) return;
    if (digit >= 1 && digit <= SCREENS.length) setIndex(digit - 1);
  });
  const Screen = SCREENS[index] ?? BlurScreen;
  return <Screen />;
}
