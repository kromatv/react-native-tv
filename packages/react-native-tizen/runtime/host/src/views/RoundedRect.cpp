#include "RoundedRect.h"

#include <algorithm>

namespace rntizen {

namespace {

constexpr int kSamples = 4;

enum class Overlap { Outside, Inside, Edge };

bool insideCorner(float x, float y, float centerX, float centerY, const CornerRadius &radius) {
  const auto dx = (x - centerX) / static_cast<float>(radius.x);
  const auto dy = (y - centerY) / static_cast<float>(radius.y);
  return dx * dx + dy * dy <= 1;
}

Overlap overlap(const RoundedRect &shape, int pixelX, int pixelY) {
  using enum Overlap;
  const auto left = static_cast<float>(pixelX) - shape.x;
  const auto top = static_cast<float>(pixelY) - shape.y;
  const auto right = left + 1;
  const auto bottom = top + 1;
  const auto w = shape.width;
  const auto h = shape.height;
  if (right <= 0 || bottom <= 0 || left >= w || top >= h) return Outside;
  if (left < 0 || top < 0 || right > w || bottom > h) return Edge;
  const auto &[tl, tr, br, bl] = shape.corners;
  const bool cornered = (left < tl.x && top < tl.y) || (right > w - tr.x && top < tr.y) ||
      (right > w - br.x && bottom > h - br.y) || (left < bl.x && bottom > h - bl.y);
  return cornered ? Edge : Inside;
}

float sampled(const RoundedRect &shape, int pixelX, int pixelY) {
  int hits = 0;
  for (int i = 0; i < kSamples; ++i) {
    for (int j = 0; j < kSamples; ++j) {
      const auto x = static_cast<float>(pixelX) + (static_cast<float>(i) + 0.5f) / kSamples;
      const auto y = static_cast<float>(pixelY) + (static_cast<float>(j) + 0.5f) / kSamples;
      hits += shape.contains(x, y) ? 1 : 0;
    }
  }
  return static_cast<float>(hits) / (kSamples * kSamples);
}

} // namespace

bool RoundedRect::contains(float px, float py) const {
  px -= x;
  py -= y;
  const auto w = width;
  const auto h = height;
  if (px < 0 || py < 0 || px > w || py > h) return false;
  const auto &[tl, tr, br, bl] = corners;
  if (px < tl.x && py < tl.y) return insideCorner(px, py, tl.x, tl.y, tl);
  if (px > w - tr.x && py < tr.y) return insideCorner(px, py, w - tr.x, tr.y, tr);
  if (px > w - br.x && py > h - br.y) return insideCorner(px, py, w - br.x, h - br.y, br);
  if (px < bl.x && py > h - bl.y) return insideCorner(px, py, bl.x, h - bl.y, bl);
  return true;
}

float RoundedRect::coverage(int pixelX, int pixelY) const {
  using enum Overlap;
  switch (overlap(*this, pixelX, pixelY)) {
    case Outside:
      return 0;
    case Inside:
      return 1;
    case Edge:
      break;
  }
  return sampled(*this, pixelX, pixelY);
}

void RoundedRect::coverageRow(int pixelY, int count, float *out) const {
  const auto top = static_cast<float>(pixelY) - y;
  const auto bottom = top + 1;
  if (bottom <= 0 || top >= height) {
    std::fill(out, out + count, 0.0f);
    return;
  }
  const bool vertical = top < 0 || bottom > height;
  const auto &[tl, tr, br, bl] = corners;
  const bool topLeft = top < tl.y;
  const bool topRight = top < tr.y;
  const bool bottomRight = bottom > height - br.y;
  const bool bottomLeft = bottom > height - bl.y;
  for (int px = 0; px < count; ++px) {
    const auto left = static_cast<float>(px) - x;
    const auto right = left + 1;
    if (right <= 0 || left >= width) {
      out[px] = 0;
      continue;
    }
    const bool cornered = (topLeft && left < tl.x) || (topRight && right > width - tr.x) ||
        (bottomRight && right > width - br.x) || (bottomLeft && left < bl.x);
    out[px] = vertical || left < 0 || right > width || cornered ? sampled(*this, px, pixelY) : 1;
  }
}

float roundedCoverage(const Corners &corners, int width, int height, int x, int y) {
  const auto &[tl, tr, br, bl] = corners;
  if (const bool inCorner = (x < tl.x && y < tl.y) || (x + 1 > width - tr.x && y < tr.y) ||
          (x + 1 > width - br.x && y + 1 > height - br.y) || (x < bl.x && y + 1 > height - bl.y);
      !inCorner) {
    return 1;
  }
  return RoundedRect{.width = static_cast<float>(width), .height = static_cast<float>(height), .corners = corners}.coverage(x, y);
}

} // namespace rntizen
