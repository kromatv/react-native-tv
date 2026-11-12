#include "ImageFit.h"

#include <algorithm>
#include <cmath>

namespace rnwebos {

namespace {

struct Scale {
  float x;
  float y;
};

// How many view pixels one image pixel covers, on each axis.
Scale drawnScale(ImageFit fit, float boxWidth, float boxHeight, float imageWidth, float imageHeight, float naturalScale) {
  const auto scaleX = boxWidth / imageWidth;
  const auto scaleY = boxHeight / imageHeight;
  using enum ImageFit;
  switch (fit) {
    case Stretch:
      return {scaleX, scaleY};
    case Cover:
      return {std::max(scaleX, scaleY), std::max(scaleX, scaleY)};
    case Contain:
      return {std::min(scaleX, scaleY), std::min(scaleX, scaleY)};
    case Center: {
      const auto scale = std::min(naturalScale, std::min(scaleX, scaleY));
      return {scale, scale};
    }
    case Natural:
    case Repeat:
      return {naturalScale, naturalScale};
  }
  return {scaleX, scaleY};
}

float wholePixels(float length) {
  return std::max(1.0f, std::round(length));
}

} // namespace

ImagePlacement placeImage(ImageFit fit, const PixelRect &box, PixelSize image, float naturalScale) {
  const auto boxRect = SkRect::MakeXYWH(
      static_cast<float>(box.x), static_cast<float>(box.y), static_cast<float>(box.width), static_cast<float>(box.height));
  const auto whole = SkRect::MakeWH(static_cast<float>(image.width), static_cast<float>(image.height));
  if (image.width <= 0 || image.height <= 0 || box.width <= 0 || box.height <= 0) {
    return {.destination = boxRect, .source = whole, .tile = {}};
  }
  const auto scale = drawnScale(fit, boxRect.width(), boxRect.height(), whole.width(), whole.height(), naturalScale);
  const auto width = wholePixels(whole.width() * scale.x);
  const auto height = wholePixels(whole.height() * scale.y);
  if (fit == ImageFit::Repeat) return {.destination = boxRect, .source = whole, .tile = {width, height}};
  // Centered on whole pixels, then cut to the box: what falls outside is not
  // painted, so a view needs no clip to keep a covering image inside.
  const auto left = boxRect.left() + std::floor((boxRect.width() - width) / 2);
  const auto top = boxRect.top() + std::floor((boxRect.height() - height) / 2);
  const auto drawn = SkRect::MakeXYWH(left, top, width, height);
  auto destination = drawn;
  if (!destination.intersect(boxRect)) return {.destination = SkRect::MakeEmpty(), .source = whole, .tile = {}};
  const auto source = SkRect::MakeLTRB(
      (destination.left() - drawn.left()) / width * whole.width(),
      (destination.top() - drawn.top()) / height * whole.height(),
      (destination.right() - drawn.left()) / width * whole.width(),
      (destination.bottom() - drawn.top()) / height * whole.height());
  return {.destination = destination, .source = source, .tile = {}};
}

PixelSize decodedImageSize(ImageFit fit, PixelSize box, PixelSize image, float naturalScale) {
  if (image.width <= 0 || image.height <= 0 || box.width <= 0 || box.height <= 0) return image;
  const auto scale = drawnScale(
      fit,
      static_cast<float>(box.width),
      static_cast<float>(box.height),
      static_cast<float>(image.width),
      static_cast<float>(image.height),
      naturalScale);
  const PixelSize drawn{
      .width = static_cast<int>(wholePixels(static_cast<float>(image.width) * scale.x)),
      .height = static_cast<int>(wholePixels(static_cast<float>(image.height) * scale.y)),
  };
  if (image.width < 2 * drawn.width || image.height < 2 * drawn.height) return image;
  return drawn;
}

} // namespace rnwebos
