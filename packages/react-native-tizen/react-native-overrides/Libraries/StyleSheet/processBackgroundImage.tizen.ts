import upstream from 'react-native/Libraries/StyleSheet/processBackgroundImage';

type Layer = ReturnType<typeof upstream>[number];

const TRANSPARENT = 0x00000001;

/**
 * React Native's `processBackgroundImage`, with `transparent` (and any color
 * that processes to 0) sent as `0x00000001` instead. On Tizen, as on Android,
 * a C++ `SharedColor` of 0 means "no color", so the props parser reads a
 * `transparent` stop as a transition hint and the gradient loses its fade
 * (`HostPlatformColor.h`, `BackgroundImagePropsConversions.cpp`). Every
 * color with alpha 0 draws the same, premultiplied, so the swap changes nothing
 * on screen.
 */
export default function processBackgroundImage(
  backgroundImage: Parameters<typeof upstream>[0],
): ReturnType<typeof upstream> {
  return upstream(backgroundImage).map((layer: Layer): Layer => ({
    ...layer,
    colorStops: layer.colorStops.map((stop) =>
      stop.color === 0 ? { ...stop, color: TRANSPARENT } : stop,
    ),
  }));
}
