#include "ImageFit.h"

#include <algorithm>
#include <cmath>

namespace rntizen {

namespace {

using facebook::react::ImageResizeMode;

Evas_Coord rounded(float value) {
  return static_cast<Evas_Coord>(std::lround(value));
}

ImagePlacement centered(const PixelRect &box, float width, float height) {
  const auto w = std::max(1, rounded(width));
  const auto h = std::max(1, rounded(height));
  return {
      .geometry = {.x = box.x + (box.width - w) / 2, .y = box.y + (box.height - h) / 2, .width = w, .height = h},
      .fill = {.x = 0, .y = 0, .width = w, .height = h},
  };
}

} // namespace

ImagePlacement placeImage(ImageResizeMode resizeMode, const PixelRect &box, int imageWidth, int imageHeight, float naturalScale) {
  const ImagePlacement stretched{.geometry = box, .fill = {.x = 0, .y = 0, .width = box.width, .height = box.height}};
  if (imageWidth <= 0 || imageHeight <= 0 || box.width <= 0 || box.height <= 0) return stretched;
  const auto iw = static_cast<float>(imageWidth);
  const auto ih = static_cast<float>(imageHeight);
  const auto fitScale = std::min(static_cast<float>(box.width) / iw, static_cast<float>(box.height) / ih);
  switch (resizeMode) {
    case ImageResizeMode::Stretch:
      return stretched;
    case ImageResizeMode::Contain:
      return centered(box, iw * fitScale, ih * fitScale);
    case ImageResizeMode::Cover: {
      const auto scale = std::max(static_cast<float>(box.width) / iw, static_cast<float>(box.height) / ih);
      const auto w = rounded(iw * scale);
      const auto h = rounded(ih * scale);
      return {.geometry = box, .fill = {.x = (box.width - w) / 2, .y = (box.height - h) / 2, .width = w, .height = h}};
    }
    case ImageResizeMode::Center:
    case ImageResizeMode::None: {
      const auto scale = std::min(naturalScale, fitScale);
      return centered(box, iw * scale, ih * scale);
    }
    case ImageResizeMode::Repeat:
      return {
          .geometry = box,
          .fill = {.x = 0, .y = 0, .width = std::max(1, rounded(iw * naturalScale)), .height = std::max(1, rounded(ih * naturalScale))}};
  }
  return stretched;
}

} // namespace rntizen
