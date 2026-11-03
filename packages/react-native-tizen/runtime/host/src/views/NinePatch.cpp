#include "NinePatch.h"

#include "AlphaBlur.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <utility>

namespace rntizen {

namespace {

constexpr int kSamples = 4;
constexpr std::size_t kCacheLimit = 256;

enum class Kind { Filled, Ring, Shadow };

using Key = std::array<int, 14>;

struct Shape {
  float x;
  float y;
  float width;
  float height;
  Corners corners;
};

bool insideCorner(float x, float y, float centerX, float centerY, const CornerRadius &radius) {
  const auto dx = (x - centerX) / static_cast<float>(radius.x);
  const auto dy = (y - centerY) / static_cast<float>(radius.y);
  return dx * dx + dy * dy <= 1;
}

bool inside(const Shape &shape, float x, float y) {
  x -= shape.x;
  y -= shape.y;
  const auto w = shape.width;
  const auto h = shape.height;
  if (x < 0 || y < 0 || x > w || y > h) return false;
  const auto &[tl, tr, br, bl] = shape.corners;
  if (x < tl.x && y < tl.y) return insideCorner(x, y, tl.x, tl.y, tl);
  if (x > w - tr.x && y < tr.y) return insideCorner(x, y, w - tr.x, tr.y, tr);
  if (x > w - br.x && y > h - br.y) return insideCorner(x, y, w - br.x, h - br.y, br);
  if (x < bl.x && y > h - bl.y) return insideCorner(x, y, bl.x, h - bl.y, bl);
  return true;
}

enum class Overlap { Outside, Inside, Edge };

Overlap overlap(const Shape &shape, int pixelX, int pixelY) {
  const auto left = static_cast<float>(pixelX) - shape.x;
  const auto top = static_cast<float>(pixelY) - shape.y;
  const auto right = left + 1;
  const auto bottom = top + 1;
  const auto w = shape.width;
  const auto h = shape.height;
  if (right <= 0 || bottom <= 0 || left >= w || top >= h) return Overlap::Outside;
  if (left < 0 || top < 0 || right > w || bottom > h) return Overlap::Edge;
  const auto &[tl, tr, br, bl] = shape.corners;
  const bool cornered = (left < tl.x && top < tl.y) || (right > w - tr.x && top < tr.y) ||
      (right > w - br.x && bottom > h - br.y) || (left < bl.x && bottom > h - bl.y);
  return cornered ? Overlap::Edge : Overlap::Inside;
}

float sampled(const Shape &shape, int pixelX, int pixelY) {
  int hits = 0;
  for (int i = 0; i < kSamples; ++i) {
    for (int j = 0; j < kSamples; ++j) {
      const auto x = static_cast<float>(pixelX) + (static_cast<float>(i) + 0.5f) / kSamples;
      const auto y = static_cast<float>(pixelY) + (static_cast<float>(j) + 0.5f) / kSamples;
      hits += inside(shape, x, y) ? 1 : 0;
    }
  }
  return static_cast<float>(hits) / (kSamples * kSamples);
}

float coverage(const Shape &shape, int pixelX, int pixelY) {
  switch (overlap(shape, pixelX, pixelY)) {
    case Overlap::Outside:
      return 0;
    case Overlap::Inside:
      return 1;
    case Overlap::Edge:
      break;
  }
  return sampled(shape, pixelX, pixelY);
}

void coverageRow(const Shape &shape, int pixelY, int count, float *out) {
  const auto top = static_cast<float>(pixelY) - shape.y;
  const auto bottom = top + 1;
  const auto w = shape.width;
  const auto h = shape.height;
  if (bottom <= 0 || top >= h) {
    std::fill(out, out + count, 0.0f);
    return;
  }
  const bool vertical = top < 0 || bottom > h;
  const auto &[tl, tr, br, bl] = shape.corners;
  const bool topLeft = top < tl.y;
  const bool topRight = top < tr.y;
  const bool bottomRight = bottom > h - br.y;
  const bool bottomLeft = bottom > h - bl.y;
  for (int x = 0; x < count; ++x) {
    const auto left = static_cast<float>(x) - shape.x;
    const auto right = left + 1;
    if (right <= 0 || left >= w) {
      out[x] = 0;
      continue;
    }
    const bool cornered = (topLeft && left < tl.x) || (topRight && right > w - tr.x) ||
        (bottomRight && right > w - br.x) || (bottomLeft && left < bl.x);
    out[x] = vertical || left < 0 || right > w || cornered ? sampled(shape, x, pixelY) : 1;
  }
}

std::uint32_t whitePixel(float alpha) {
  const auto a = static_cast<std::uint32_t>(static_cast<double>(std::clamp(alpha, 0.0f, 1.0f) * 255) + 0.5);
  return a << 24 | a << 16 | a << 8 | a;
}

Edges cornerExtents(const Corners &c) {
  return {
      .left = std::max(c.topLeft.x, c.bottomLeft.x),
      .top = std::max(c.topLeft.y, c.topRight.y),
      .right = std::max(c.topRight.x, c.bottomRight.x),
      .bottom = std::max(c.bottomLeft.y, c.bottomRight.y),
  };
}

Key keyFor(Kind kind, const Corners &c, const Edges &edges, int blur) {
  return {static_cast<int>(kind), c.topLeft.x, c.topLeft.y, c.topRight.x, c.topRight.y, c.bottomRight.x,
          c.bottomRight.y, c.bottomLeft.x, c.bottomLeft.y, edges.left, edges.top, edges.right, edges.bottom, blur};
}

NinePatch blankPatch(const Edges &border) {
  NinePatch patch{.width = border.left + border.right + 1, .height = border.top + border.bottom + 1, .border = border};
  patch.pixels.resize(static_cast<std::size_t>(patch.width) * static_cast<std::size_t>(patch.height));
  return patch;
}

template <typename Alpha>
void paint(NinePatch &patch, Alpha alphaAt) {
  for (int y = 0; y < patch.height; ++y) {
    for (int x = 0; x < patch.width; ++x) patch.pixels[static_cast<std::size_t>(y * patch.width + x)] = whitePixel(alphaAt(x, y));
  }
}

NinePatch makeFilled(const Corners &corners) {
  auto patch = blankPatch(cornerExtents(corners));
  const Shape shape{0, 0, static_cast<float>(patch.width), static_cast<float>(patch.height), corners};
  paint(patch, [&](int x, int y) { return coverage(shape, x, y); });
  return patch;
}

NinePatch makeRing(const Corners &corners, const Edges &widths) {
  const auto extents = cornerExtents(corners);
  auto patch = blankPatch({
      .left = std::max(extents.left, widths.left),
      .top = std::max(extents.top, widths.top),
      .right = std::max(extents.right, widths.right),
      .bottom = std::max(extents.bottom, widths.bottom),
  });
  const auto w = static_cast<float>(patch.width);
  const auto h = static_cast<float>(patch.height);
  auto inner = [](const CornerRadius &r, int dx, int dy) { return CornerRadius{std::max(0, r.x - dx), std::max(0, r.y - dy)}; };
  const Shape outer{0, 0, w, h, corners};
  const Shape hole{
      static_cast<float>(widths.left),
      static_cast<float>(widths.top),
      w - static_cast<float>(widths.left + widths.right),
      h - static_cast<float>(widths.top + widths.bottom),
      {inner(corners.topLeft, widths.left, widths.top), inner(corners.topRight, widths.right, widths.top),
       inner(corners.bottomRight, widths.right, widths.bottom), inner(corners.bottomLeft, widths.left, widths.bottom)}};
  paint(patch, [&](int x, int y) { return coverage(outer, x, y) - coverage(hole, x, y); });
  return patch;
}

NinePatch makeShadow(const Corners &corners, int blur) {
  const auto pad = shadowPadding(blur);
  const auto extents = cornerExtents(corners);
  auto patch = blankPatch({
      .left = 2 * pad + extents.left,
      .top = 2 * pad + extents.top,
      .right = 2 * pad + extents.right,
      .bottom = 2 * pad + extents.bottom,
  });
  const Shape shape{
      static_cast<float>(pad),
      static_cast<float>(pad),
      static_cast<float>(patch.width - 2 * pad),
      static_cast<float>(patch.height - 2 * pad),
      corners};
  std::vector<float> alpha(patch.pixels.size());
  for (int y = 0; y < patch.height; ++y) {
    coverageRow(shape, y, patch.width, alpha.data() + static_cast<std::size_t>(y) * static_cast<std::size_t>(patch.width));
  }
  if (blur > 0) alpha = blurredAlpha(std::move(alpha), patch.width, patch.height, pad, static_cast<float>(blur) / 2);
  paint(patch, [&](int x, int y) { return alpha[static_cast<std::size_t>(y * patch.width + x)]; });
  return patch;
}

template <typename Make>
const NinePatch &cached(const Key &key, Make make) {
  static std::map<Key, NinePatch> cache;
  if (auto found = cache.find(key); found != cache.end()) return found->second;
  if (cache.size() >= kCacheLimit) cache.clear();
  return cache.emplace(key, make()).first->second;
}

} // namespace

bool Corners::isSquare() const {
  return *this == Corners{};
}

Corners Corners::grown(int amount) const {
  auto grow = [amount](const CornerRadius &r) {
    if (r.x == 0 && r.y == 0) return r;
    return CornerRadius{std::max(0, r.x + amount), std::max(0, r.y + amount)};
  };
  return {grow(topLeft), grow(topRight), grow(bottomRight), grow(bottomLeft)};
}

bool Edges::isZero() const {
  return *this == Edges{};
}

float roundedCoverage(const Corners &corners, int width, int height, int x, int y) {
  const auto &[tl, tr, br, bl] = corners;
  const bool inCorner = (x < tl.x && y < tl.y) || (x + 1 > width - tr.x && y < tr.y) ||
      (x + 1 > width - br.x && y + 1 > height - br.y) || (x < bl.x && y + 1 > height - bl.y);
  if (!inCorner) return 1;
  return coverage({0, 0, static_cast<float>(width), static_cast<float>(height), corners}, x, y);
}

bool insideRounded(const Corners &corners, float left, float top, float width, float height, float x, float y) {
  return inside({left, top, width, height, corners}, x, y);
}

int shadowPadding(int blur) {
  return blur <= 0 ? 0 : static_cast<int>(std::ceil(static_cast<float>(blur) * 1.5f));
}

const NinePatch &filledPatch(const Corners &corners) {
  return cached(keyFor(Kind::Filled, corners, {}, 0), [&] { return makeFilled(corners); });
}

const NinePatch &ringPatch(const Corners &corners, const Edges &widths) {
  return cached(keyFor(Kind::Ring, corners, widths, 0), [&] { return makeRing(corners, widths); });
}

NinePatch makeShadowPatch(const Corners &corners, int blur) {
  return makeShadow(corners, blur);
}

} // namespace rntizen
