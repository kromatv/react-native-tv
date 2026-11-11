#include "VideoFit.h"

#include <algorithm>
#include <cmath>

namespace rnwebos::expo {

namespace {

struct Box {
  double x;
  double y;
  double width;
  double height;
};

Box fittedFrame(const PixelRect &view, int videoWidth, int videoHeight, ContentFit fit) {
  const Box whole{double(view.x), double(view.y), double(view.width), double(view.height)};
  if (fit == ContentFit::Fill || videoWidth <= 0 || videoHeight <= 0) return whole;
  const double scaleX = whole.width / videoWidth;
  const double scaleY = whole.height / videoHeight;
  const double scale = fit == ContentFit::Cover ? std::max(scaleX, scaleY) : std::min(scaleX, scaleY);
  const double width = videoWidth * scale;
  const double height = videoHeight * scale;
  return {whole.x + (whole.width - width) / 2, whole.y + (whole.height - height) / 2, width, height};
}

Box intersect(const Box &a, const Box &b) {
  const double left = std::max(a.x, b.x);
  const double top = std::max(a.y, b.y);
  const double right = std::min(a.x + a.width, b.x + b.width);
  const double bottom = std::min(a.y + a.height, b.y + b.height);
  return {left, top, std::max(0.0, right - left), std::max(0.0, bottom - top)};
}

Box boxOf(const PixelRect &rect) {
  return {double(rect.x), double(rect.y), double(rect.width), double(rect.height)};
}

int pixel(double value) {
  return static_cast<int>(std::lround(value));
}

} // namespace

std::optional<PlaneGeometry> fitVideo(
    const PixelRect &view, const PixelRect &window, int videoWidth, int videoHeight, ContentFit fit) {
  const auto frame = fittedFrame(view, videoWidth, videoHeight, fit);
  if (frame.width < 1 || frame.height < 1) return std::nullopt;
  const auto shown = intersect(intersect(frame, boxOf(view)), boxOf(window));
  const PixelRect roi{
      .x = pixel(shown.x),
      .y = pixel(shown.y),
      .width = pixel(shown.x + shown.width) - pixel(shown.x),
      .height = pixel(shown.y + shown.height) - pixel(shown.y),
  };
  if (roi.width < 1 || roi.height < 1) return std::nullopt;
  const SourceCrop crop{
      .x = (shown.x - frame.x) / frame.width,
      .y = (shown.y - frame.y) / frame.height,
      .width = shown.width / frame.width,
      .height = shown.height / frame.height,
  };
  return PlaneGeometry{.roi = roi, .crop = crop};
}

PixelRect sourcePixels(const SourceCrop &crop, int videoWidth, int videoHeight) {
  if (videoWidth <= 0 || videoHeight <= 0) return {};
  const int left = pixel(crop.x * videoWidth);
  const int top = pixel(crop.y * videoHeight);
  return {
      .x = left,
      .y = top,
      .width = std::max(1, pixel((crop.x + crop.width) * videoWidth) - left),
      .height = std::max(1, pixel((crop.y + crop.height) * videoHeight) - top),
  };
}

} // namespace rnwebos::expo
