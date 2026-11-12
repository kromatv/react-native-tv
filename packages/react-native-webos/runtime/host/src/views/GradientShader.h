#pragma once

#include <react/renderer/graphics/BackgroundImage.h>

#include "include/core/SkShader.h"

namespace rnwebos {

/// The Skia shader that paints `image`, a CSS gradient as React Native parsed
/// it, over a box of `width` x `height` pixels with its corner at the origin,
/// at `pointScale` pixels per point. Null when the gradient has no color or
/// the box no area. Recent shaders are kept, so asking again for the same
/// gradient and box allocates nothing. Main thread only.
const sk_sp<SkShader> &
gradientShader(const facebook::react::BackgroundImage &image, float width, float height, float pointScale);

/// What `gradientShader` returns, made anew.
sk_sp<SkShader>
makeGradientShader(const facebook::react::BackgroundImage &image, float width, float height, float pointScale);

} // namespace rnwebos
