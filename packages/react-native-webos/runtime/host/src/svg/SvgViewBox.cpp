#include "SvgViewBox.h"

#include <algorithm>
#include <string_view>

namespace rnwebos::svg {

namespace {

constexpr int kMeet = 0;
constexpr int kSlice = 1;
constexpr int kNone = 2;

bool contains(std::string_view text, std::string_view part) {
  return text.find(part) != std::string_view::npos;
}

} // namespace

bool SvgViewBox::present() const {
  return !align.empty() && width > 0 && height > 0;
}

SkMatrix viewBoxTransform(const SvgViewBox &viewBox, float width, float height) {
  if (!viewBox.present()) return SkMatrix::I();
  float scaleX = width / viewBox.width;
  float scaleY = height / viewBox.height;

  if (viewBox.meetOrSlice == kNone) {
    const float scale = std::min(scaleX, scaleY);
    float translateX = -viewBox.minX * scaleX;
    float translateY = -viewBox.minY * scaleY;
    if (scale > 1) {
      translateX -= (width / scale - viewBox.width) / 2;
      translateY -= (height / scale - viewBox.height) / 2;
    } else {
      translateX -= (width - viewBox.width * scale) / 2;
      translateY -= (height - viewBox.height * scale) / 2;
    }
    return SkMatrix::ScaleTranslate(scale, scale, translateX, translateY);
  }

  if (viewBox.align != "none" && viewBox.meetOrSlice == kMeet) {
    scaleY = std::min(scaleX, scaleY);
    scaleX = scaleY;
  }
  if (viewBox.align != "none" && viewBox.meetOrSlice == kSlice) {
    scaleY = std::max(scaleX, scaleY);
    scaleX = scaleY;
  }
  float translateX = -viewBox.minX * scaleX;
  float translateY = -viewBox.minY * scaleY;
  if (contains(viewBox.align, "xMid")) translateX += (width - viewBox.width * scaleX) / 2;
  if (contains(viewBox.align, "xMax")) translateX += width - viewBox.width * scaleX;
  if (contains(viewBox.align, "YMid")) translateY += (height - viewBox.height * scaleY) / 2;
  if (contains(viewBox.align, "YMax")) translateY += height - viewBox.height * scaleY;
  return SkMatrix::ScaleTranslate(scaleX, scaleY, translateX, translateY);
}

} // namespace rnwebos::svg
