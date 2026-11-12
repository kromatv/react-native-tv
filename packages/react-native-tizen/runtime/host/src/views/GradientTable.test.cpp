#include "GradientTable.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>

namespace rntizen {

namespace {

using facebook::react::colorFromRGBA;
using facebook::react::LinearGradient;

GradientGeometry diagonal(float width, float height) {
  return {LinearGradient{.direction = 30.0f, .colorStops = {}}, width, height, 1};
}

GradientRamp ramp(float lineLength) {
  return {
      {{.color = colorFromRGBA(255, 0, 0, 255), .position = {}},
       {.color = colorFromRGBA(0, 128, 255, 128), .position = {}},
       {.color = colorFromRGBA(0, 255, 0, 255), .position = {}}},
      lineLength,
      1};
}

int largestChannelStep(std::uint32_t a, std::uint32_t b) {
  int largest = 0;
  for (int shift = 0; shift < 32; shift += 8) {
    const auto difference = std::abs(static_cast<int>((a >> shift) & 0xff) - static_cast<int>((b >> shift) & 0xff));
    largest = std::max(largest, difference);
  }
  return largest;
}

} // namespace

TEST_CASE("GradientTable differs from the ramp by one step of 255 at most across the patch") {
  const auto geometry = diagonal(300, 120);
  const auto colors = ramp(geometry.lineLength());
  const GradientTable table(geometry, colors, 300, 120, 1, 1);
  REQUIRE_FALSE(table.empty());

  int largest = 0;
  for (int y = 0; y < 120; y += 3) {
    for (int x = 0; x < 300; x += 3) {
      const auto t = geometry.at(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f);
      largest = std::max(largest, largestChannelStep(table.at(t), pixelOf(colors.at(t))));
    }
  }

  CHECK(largest <= 1);
}

TEST_CASE("GradientTable starts at the line's first color and holds its last entry past the patch") {
  const auto geometry = diagonal(100, 100);
  const auto colors = ramp(geometry.lineLength());
  const GradientTable table(geometry, colors, 100, 100, 1, 1);

  CHECK(table.at(-10) == pixelOf(colors.at(0)));
  CHECK(table.at(10) == table.at(100));
}

TEST_CASE("GradientTable is empty when its line would need too many entries") {
  const auto geometry = diagonal(10000, 10000);
  const GradientTable table(geometry, ramp(geometry.lineLength()), 10000, 10000, 1, 1);

  CHECK(table.empty());
}

TEST_CASE("GradientTable is empty for a line that does not change") {
  const GradientGeometry flat(LinearGradient{.direction = 30.0f, .colorStops = {}}, 0, 0, 1);

  CHECK(GradientTable(flat, ramp(0), 1, 1, 1, 1).empty());
}

} // namespace rntizen
