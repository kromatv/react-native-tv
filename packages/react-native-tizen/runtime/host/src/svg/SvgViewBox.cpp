#include "SvgViewBox.h"

#include <algorithm>
#include <string_view>

namespace rntizen::svg {

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

Affine viewBoxTransform(const SvgViewBox &viewBox, double width, double height) {
  if (!viewBox.present()) return {};
  double scaleX = width / viewBox.width;
  double scaleY = height / viewBox.height;

  if (viewBox.meetOrSlice == kNone) {
    const double scale = std::min(scaleX, scaleY);
    double translateX = -viewBox.minX * scaleX;
    double translateY = -viewBox.minY * scaleY;
    if (scale > 1) {
      translateX -= (width / scale - viewBox.width) / 2;
      translateY -= (height / scale - viewBox.height) / 2;
    } else {
      translateX -= (width - viewBox.width * scale) / 2;
      translateY -= (height - viewBox.height * scale) / 2;
    }
    return Affine::translation(translateX, translateY) * Affine::scaling(scale, scale);
  }

  if (viewBox.align != "none" && viewBox.meetOrSlice == kMeet) {
    scaleY = std::min(scaleX, scaleY);
    scaleX = scaleY;
  }
  if (viewBox.align != "none" && viewBox.meetOrSlice == kSlice) {
    scaleY = std::max(scaleX, scaleY);
    scaleX = scaleY;
  }
  double translateX = -viewBox.minX * scaleX;
  double translateY = -viewBox.minY * scaleY;
  if (contains(viewBox.align, "xMid")) translateX += (width - viewBox.width * scaleX) / 2;
  if (contains(viewBox.align, "xMax")) translateX += width - viewBox.width * scaleX;
  if (contains(viewBox.align, "YMid")) translateY += (height - viewBox.height * scaleY) / 2;
  if (contains(viewBox.align, "YMax")) translateY += height - viewBox.height * scaleY;
  return Affine::translation(translateX, translateY) * Affine::scaling(scaleX, scaleY);
}

} // namespace rntizen::svg
