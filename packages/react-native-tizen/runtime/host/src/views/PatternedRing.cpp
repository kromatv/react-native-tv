#include "PatternedRing.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <numbers>

namespace rntizen {

namespace {

constexpr int kMaxSide = 2048;
constexpr int kSamples = 4;
constexpr std::size_t kCacheBytes = 8 << 20;
constexpr float kPi = std::numbers::pi_v<float>;

using Key = std::array<int, 15>;

struct Arc {
  float cx;
  float cy;
  float a;
  float b;
  float length;
};

float quarterLength(float a, float b) {
  return kPi / 4 * (3 * (a + b) - std::sqrt((3 * a + b) * (a + 3 * b)));
}

Arc arc(float cx, float cy, float a, float b) {
  return {cx, cy, std::max(0.0f, a), std::max(0.0f, b), quarterLength(std::max(0.0f, a), std::max(0.0f, b))};
}

float bestGap(float length, float dash, float gap) {
  const auto fewer = std::floor(length / (dash + gap));
  const auto more = fewer + 1;
  const auto fewerGap = (length - fewer * dash) / fewer;
  const auto moreGap = (length - more * dash) / more;
  return moreGap <= 0 || std::abs(fewerGap - gap) < std::abs(moreGap - gap) ? fewerGap : moreGap;
}

class Outline {
 public:
  Outline(const Corners &c, const Edges &w, float width, float height) : width_(width), height_(height), widths_(w) {
    const auto wl = static_cast<float>(w.left) / 2;
    const auto wt = static_cast<float>(w.top) / 2;
    const auto wr = static_cast<float>(w.right) / 2;
    const auto wb = static_cast<float>(w.bottom) / 2;
    auto at = [](int radius, float half) { return std::max(static_cast<float>(radius), half); };
    const auto tlX = at(c.topLeft.x, wl);
    const auto tlY = at(c.topLeft.y, wt);
    const auto trX = width - at(c.topRight.x, wr);
    const auto trY = at(c.topRight.y, wt);
    const auto brX = width - at(c.bottomRight.x, wr);
    const auto brY = height - at(c.bottomRight.y, wb);
    const auto blX = at(c.bottomLeft.x, wl);
    const auto blY = height - at(c.bottomLeft.y, wb);
    arcs_ = {arc(tlX, tlY, tlX - wl, tlY - wt), arc(trX, trY, width - wr - trX, trY - wt),
             arc(brX, brY, width - wr - brX, height - wb - brY), arc(blX, blY, blX - wl, height - wb - blY)};
    sides_ = {trX - tlX, brY - trY, brX - blX, blY - tlY};
    total_ = 0;
    for (std::size_t i = 0; i < 4; ++i) total_ += sides_[i] + arcs_[i].length;
  }

  float length() const {
    return total_;
  }

  std::array<float, 2> locate(float x, float y) const {
    const auto &[tl, tr, br, bl] = arcs_;
    if (x < tl.cx && y < tl.cy) return onArc(tl, x, y, kPi, start(3) + sides_[3]);
    if (x > tr.cx && y < tr.cy) return onArc(tr, x, y, -kPi / 2, start(0) + sides_[0]);
    if (x > br.cx && y > br.cy) return onArc(br, x, y, 0, start(1) + sides_[1]);
    if (x < bl.cx && y > bl.cy) return onArc(bl, x, y, kPi / 2, start(2) + sides_[2]);
    const std::array<float, 4> distances{y, width_ - x, height_ - y, x};
    const auto side = static_cast<std::size_t>(std::min_element(distances.begin(), distances.end()) - distances.begin());
    const std::array<float, 4> along{x - tl.cx, y - tr.cy, br.cx - x, bl.cy - y};
    const std::array<float, 4> half{
        static_cast<float>(widths_.top) / 2, static_cast<float>(widths_.right) / 2, static_cast<float>(widths_.bottom) / 2,
        static_cast<float>(widths_.left) / 2};
    return {wrap(start(side) + std::clamp(along[side], 0.0f, sides_[side])), distances[side] - half[side]};
  }

 private:
  float start(std::size_t side) const {
    float s = arcs_[0].length / 2;
    for (std::size_t i = 0; i < side; ++i) s += sides_[i] + arcs_[i + 1].length;
    return s;
  }

  float wrap(float s) const {
    return total_ > 0 ? std::fmod(s, total_) : 0;
  }

  std::array<float, 2> onArc(const Arc &corner, float x, float y, float from, float s0) const {
    const auto dx = x - corner.cx;
    const auto dy = y - corner.cy;
    auto t = std::atan2(dy / std::max(corner.b, 1e-3f), dx / std::max(corner.a, 1e-3f));
    if (t < from) t += 2 * kPi;
    const auto f = std::clamp((t - from) / (kPi / 2), 0.0f, 1.0f);
    const auto radius = std::hypot(corner.a * std::cos(t), corner.b * std::sin(t));
    return {wrap(s0 + f * corner.length), std::hypot(dx, dy) - radius};
  }

  float width_;
  float height_;
  Edges widths_;
  std::array<Arc, 4> arcs_{};
  std::array<float, 4> sides_{};
  float total_{0};
};

CornerRadius inner(const CornerRadius &r, int dx, int dy) {
  return {std::max(0, r.x - dx), std::max(0, r.y - dy)};
}

NinePatch paint(const Corners &c, const Edges &w, int width, int height, StrokePattern pattern) {
  const Outline outline(c, w, static_cast<float>(width), static_cast<float>(height));
  const auto thickness = static_cast<float>(std::max({w.left, w.top, w.right, w.bottom}));
  const auto total = outline.length();
  const bool dotted = pattern == StrokePattern::Dotted;
  auto dash = dotted ? thickness : thickness * (thickness >= 3 ? 2 : 3);
  auto gap = dotted ? thickness : thickness * (thickness >= 3 ? 1 : 2);
  if (total <= 2 * dash) return {};
  if (total <= 2 * (dash + gap)) {
    const auto shrink = total / (2 * (dash + gap));
    dash *= shrink;
    gap *= shrink;
  } else {
    gap = bestGap(total, dash, gap);
  }
  const auto period = dash + gap;
  const Corners hole{inner(c.topLeft, w.left, w.top), inner(c.topRight, w.right, w.top),
                     inner(c.bottomRight, w.right, w.bottom), inner(c.bottomLeft, w.left, w.bottom)};
  const auto fw = static_cast<float>(width);
  const auto fh = static_cast<float>(height);
  auto shown = [&](float x, float y) {
    if (!insideRounded(c, 0, 0, fw, fh, x, y)) return false;
    if (insideRounded(hole, static_cast<float>(w.left), static_cast<float>(w.top),
                      fw - static_cast<float>(w.left + w.right), fh - static_cast<float>(w.top + w.bottom), x, y)) {
      return false;
    }
    const auto [s, off] = outline.locate(x, y);
    if (!dotted) return std::fmod(s + dash / 2, period) < dash;
    const auto along = std::min(std::fmod(s, period), period - std::fmod(s, period));
    return along * along + off * off <= dash * dash / 4;
  };
  const auto top = std::max({w.top, c.topLeft.y, c.topRight.y});
  const auto bottom = height - std::max({w.bottom, c.bottomLeft.y, c.bottomRight.y});
  const auto left = std::max({w.left, c.topLeft.x, c.bottomLeft.x});
  const auto right = width - std::max({w.right, c.topRight.x, c.bottomRight.x});
  NinePatch patch{.width = width, .height = height};
  patch.pixels.resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height));
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (y >= top && y < bottom && x >= left && x < right) continue;
      int hits = 0;
      for (int i = 0; i < kSamples; ++i) {
        for (int j = 0; j < kSamples; ++j) {
          hits += shown(static_cast<float>(x) + (static_cast<float>(i) + 0.5f) / kSamples,
                        static_cast<float>(y) + (static_cast<float>(j) + 0.5f) / kSamples)
              ? 1
              : 0;
        }
      }
      const auto a = static_cast<std::uint32_t>(std::lround(255.0f * static_cast<float>(hits) / (kSamples * kSamples)));
      patch.pixels[static_cast<std::size_t>(y * width + x)] = a << 24 | a << 16 | a << 8 | a;
    }
  }
  return patch;
}

} // namespace

const NinePatch *patternedRingPatch(
    const Corners &corners,
    const Edges &widths,
    int width,
    int height,
    StrokePattern pattern) {
  if (pattern == StrokePattern::Solid || width <= 0 || height <= 0 || width > kMaxSide || height > kMaxSide) return nullptr;
  static std::map<Key, NinePatch> cache;
  static std::size_t bytes = 0;
  const auto &c = corners;
  const Key key{static_cast<int>(pattern), width, height, c.topLeft.x, c.topLeft.y, c.topRight.x, c.topRight.y,
                c.bottomRight.x, c.bottomRight.y, c.bottomLeft.x, c.bottomLeft.y, widths.left, widths.top,
                widths.right, widths.bottom};
  if (auto found = cache.find(key); found != cache.end()) return found->second.pixels.empty() ? nullptr : &found->second;
  auto patch = paint(corners, widths, width, height, pattern);
  const auto size = patch.pixels.size() * sizeof(std::uint32_t);
  if (bytes + size > kCacheBytes) {
    cache.clear();
    bytes = 0;
  }
  bytes += size;
  const auto &stored = cache.emplace(key, std::move(patch)).first->second;
  return stored.pixels.empty() ? nullptr : &stored;
}

} // namespace rntizen
