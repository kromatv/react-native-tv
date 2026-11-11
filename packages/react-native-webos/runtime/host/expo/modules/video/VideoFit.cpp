#include "VideoFit.h"

#include <algorithm>
#include <cmath>

namespace rntizen::expo {

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

} // namespace

std::optional<PlaneGeometry> fitVideo(
    const PixelRect &view, const PixelRect &window, int videoWidth, int videoHeight, ContentFit fit) {
  const auto frame = fittedFrame(view, videoWidth, videoHeight, fit);
  if (frame.width < 1 || frame.height < 1) return std::nullopt;
  const auto shown = intersect(intersect(frame, boxOf(view)), boxOf(window));
  const PixelRect roi{
      .x = static_cast<Evas_Coord>(std::lround(shown.x)),
      .y = static_cast<Evas_Coord>(std::lround(shown.y)),
      .width = static_cast<Evas_Coord>(std::lround(shown.x + shown.width)) - static_cast<Evas_Coord>(std::lround(shown.x)),
      .height =
          static_cast<Evas_Coord>(std::lround(shown.y + shown.height)) - static_cast<Evas_Coord>(std::lround(shown.y)),
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

} // namespace rntizen::expo
