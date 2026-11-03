#include "GradientImage.h"

#include "GradientGeometry.h"
#include "GradientRamp.h"
#include "GradientTable.h"
#include "PatchCache.h"
#include "RoundedRect.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace rntizen {

namespace {

using facebook::react::LinearGradient;
using facebook::react::RadialGradient;

constexpr int kMaxSide = 2048;
constexpr float kMaxPixels = 512 * 512;

void appendUnit(PatchKey &key, const std::optional<facebook::react::ValueUnit> &value) {
  if (!value) {
    key.push_back(-1);
    return;
  }
  key.push_back(value->value);
  key.push_back(static_cast<float>(value->unit));
}

void appendStops(PatchKey &key, const std::vector<facebook::react::ColorStop> &stops) {
  for (const auto &stop : stops) {
    if (stop.color) {
      const auto c = facebook::react::colorComponentsFromColor(stop.color);
      key.insert(key.end(), {c.red, c.green, c.blue, c.alpha});
    } else {
      key.push_back(-1);
    }
    appendUnit(key, stop.position);
  }
}

void appendImage(PatchKey &key, const LinearGradient &linear) {
  key.push_back(0);
  key.push_back(static_cast<float>(linear.direction.index()));
  if (const auto *angle = std::get_if<facebook::react::Float>(&linear.direction)) {
    key.push_back(*angle);
  } else {
    key.push_back(static_cast<float>(std::get<facebook::react::GradientKeyword>(linear.direction)));
  }
  appendStops(key, linear.colorStops);
}

void appendImage(PatchKey &key, const RadialGradient &radial) {
  key.push_back(1);
  key.push_back(static_cast<float>(radial.shape));
  if (const auto *size = std::get_if<facebook::react::RadialGradientSize::Dimensions>(&radial.size.value)) {
    appendUnit(key, size->x);
    appendUnit(key, size->y);
  } else {
    key.push_back(static_cast<float>(std::get<facebook::react::RadialGradientSize::SizeKeyword>(radial.size.value)));
  }
  for (const auto *side : {&radial.position.top, &radial.position.left, &radial.position.right, &radial.position.bottom}) {
    appendUnit(key, *side);
  }
  appendStops(key, radial.colorStops);
}

PatchKey keyFor(const facebook::react::BackgroundImage &image, const GradientBox &box) {
  auto key = patchKey(PatchKind::Gradient, box.corners);
  key.insert(key.end(), {static_cast<float>(box.width), static_cast<float>(box.height), box.pointScale});
  std::visit([&key](const auto &gradient) { appendImage(key, gradient); }, image);
  return key;
}

const std::vector<facebook::react::ColorStop> &stopsOf(const facebook::react::BackgroundImage &image) {
  return std::visit([](const auto &gradient) -> const auto & { return gradient.colorStops; }, image);
}

Corners scaled(const Corners &corners, float sx, float sy) {
  auto scale = [sx, sy](const CornerRadius &r) {
    return CornerRadius{static_cast<int>(std::lround(static_cast<float>(r.x) * sx)), static_cast<int>(std::lround(static_cast<float>(r.y) * sy))};
  };
  return {scale(corners.topLeft), scale(corners.topRight), scale(corners.bottomRight), scale(corners.bottomLeft)};
}

std::uint32_t covered(
    const Premultiplied &color, const Corners &corners, bool square, int width, int height, int x, int y) {
  if (square) return pixelOf(color);
  const auto coverage = roundedCoverage(corners, width, height, x, y);
  if (coverage == 1) return pixelOf(color);
  return pixelOf({color[0] * coverage, color[1] * coverage, color[2] * coverage, color[3] * coverage});
}

NinePatch strip(const GradientGeometry &geometry, const GradientRamp &ramp, const GradientBox &box, bool vertical) {
  const auto &c = box.corners;
  const auto along = vertical ? box.height : box.width;
  const auto length = std::min(along, kMaxSide);
  const auto stretch = static_cast<float>(length) / static_cast<float>(along);
  const Edges border = vertical
      ? Edges{.left = std::max(c.topLeft.x, c.bottomLeft.x), .right = std::max(c.topRight.x, c.bottomRight.x)}
      : Edges{.top = std::max(c.topLeft.y, c.topRight.y), .bottom = std::max(c.bottomLeft.y, c.bottomRight.y)};
  const auto across = vertical ? border.left + border.right + 1 : border.top + border.bottom + 1;
  NinePatch patch{.width = vertical ? across : length, .height = vertical ? length : across, .border = border};
  patch.pixels.resize(static_cast<std::size_t>(patch.width) * static_cast<std::size_t>(patch.height));
  const auto corners = vertical ? scaled(c, 1, stretch) : scaled(c, stretch, 1);
  const bool square = corners.isSquare();
  for (int i = 0; i < length; ++i) {
    const auto point = (static_cast<float>(i) + 0.5f) / stretch;
    const auto color = ramp.at(
        vertical ? geometry.at(static_cast<float>(box.width) / 2, point)
                 : geometry.at(point, static_cast<float>(box.height) / 2));
    for (int j = 0; j < across; ++j) {
      const auto x = vertical ? j : i;
      const auto y = vertical ? i : j;
      const auto pixel = covered(color, corners, square, patch.width, patch.height, x, y);
      patch.pixels[static_cast<std::size_t>(y * patch.width + x)] = pixel;
    }
  }
  return patch;
}

NinePatch whole(const GradientGeometry &geometry, const GradientRamp &ramp, const GradientBox &box) {
  const auto width = static_cast<float>(box.width);
  const auto height = static_cast<float>(box.height);
  const auto scale = std::min({1.0f, std::sqrt(kMaxPixels / (width * height)), kMaxSide / width, kMaxSide / height});
  NinePatch patch{
      .width = std::max(1, static_cast<int>(std::lround(width * scale))),
      .height = std::max(1, static_cast<int>(std::lround(height * scale)))};
  patch.pixels.resize(static_cast<std::size_t>(patch.width) * static_cast<std::size_t>(patch.height));
  const auto sx = static_cast<float>(patch.width) / width;
  const auto sy = static_cast<float>(patch.height) / height;
  const auto corners = scaled(box.corners, sx, sy);
  const bool square = corners.isSquare();
  const GradientTable table(geometry, ramp, patch.width, patch.height, sx, sy);
  std::size_t segment = 0;
  std::vector<float> fractions(static_cast<std::size_t>(patch.width));
  for (int y = 0; y < patch.height; ++y) {
    geometry.row(0.5f / sx, 1 / sx, (static_cast<float>(y) + 0.5f) / sy, patch.width, fractions.data());
    auto *row = patch.pixels.data() + static_cast<std::size_t>(y) * static_cast<std::size_t>(patch.width);
    for (int x = 0; x < patch.width; ++x) {
      const auto t = fractions[static_cast<std::size_t>(x)];
      const auto coverage = square ? 1 : roundedCoverage(corners, patch.width, patch.height, x, y);
      if (coverage == 1 && !table.empty()) {
        row[x] = table.at(t);
        continue;
      }
      const auto color = ramp.at(t, segment);
      row[x] = pixelOf({color[0] * coverage, color[1] * coverage, color[2] * coverage, color[3] * coverage});
    }
  }
  return patch;
}

NinePatch paint(const facebook::react::BackgroundImage &image, const GradientBox &box) {
  if (box.width <= 0 || box.height <= 0) return {};
  const GradientGeometry geometry(image, static_cast<float>(box.width), static_cast<float>(box.height), box.pointScale);
  const GradientRamp ramp(stopsOf(image), geometry.lineLength(), box.pointScale);
  if (ramp.empty()) return {};
  using enum GradientGeometry::Axis;
  switch (geometry.axis()) {
    case Vertical:
      return strip(geometry, ramp, box, true);
    case Horizontal:
      return strip(geometry, ramp, box, false);
    case None:
      return whole(geometry, ramp, box);
  }
  return {};
}

} // namespace

const NinePatch &gradientPatch(const facebook::react::BackgroundImage &image, const GradientBox &box) {
  return cachedPatch(keyFor(image, box), [&image, &box] { return paint(image, box); });
}

} // namespace rntizen
