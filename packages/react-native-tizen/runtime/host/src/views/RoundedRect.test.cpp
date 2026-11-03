#include "RoundedRect.h"

#include <doctest/doctest.h>

#include <array>

namespace rntizen {

namespace {

constexpr Corners kRound8{
    .topLeft = {8, 8},
    .topRight = {8, 8},
    .bottomRight = {8, 8},
    .bottomLeft = {8, 8},
};

} // namespace

TEST_CASE("RoundedRect contains its middle and excludes its cut corners") {
  const RoundedRect shape{.width = 40, .height = 20, .corners = kRound8};

  CHECK(shape.contains(20, 10));
  CHECK(shape.contains(0, 10));
  CHECK_FALSE(shape.contains(0.5f, 0.5f));
  CHECK_FALSE(shape.contains(39.5f, 19.5f));
  CHECK_FALSE(shape.contains(41, 10));
}

TEST_CASE("RoundedRect contains points relative to its origin") {
  const RoundedRect shape{.x = 100, .y = 50, .width = 10, .height = 10};

  CHECK(shape.contains(105, 55));
  CHECK_FALSE(shape.contains(5, 5));
}

TEST_CASE("RoundedRect coverage is whole inside, zero outside and partial on a corner") {
  const RoundedRect shape{.width = 40, .height = 20, .corners = kRound8};

  CHECK(shape.coverage(20, 10) == 1.0f);
  CHECK(shape.coverage(-1, 10) == 0.0f);
  CHECK(shape.coverage(40, 10) == 0.0f);
  CHECK(shape.coverage(0, 0) == 0.0f);
  const auto edge = shape.coverage(1, 2);
  CHECK(edge > 0.0f);
  CHECK(edge < 1.0f);
}

TEST_CASE("RoundedRect coverage samples a pixel the box edge cuts") {
  const RoundedRect shape{.x = 0.5f, .y = 0, .width = 10, .height = 10};

  CHECK(shape.coverage(0, 5) == doctest::Approx(0.5f));
}

TEST_CASE("RoundedRect coverageRow matches coverage pixel by pixel") {
  const RoundedRect shape{.x = 2.25f, .y = 1.5f, .width = 30, .height = 12, .corners = kRound8};
  std::array<float, 36> row{};

  for (int y = 0; y < 16; ++y) {
    shape.coverageRow(y, static_cast<int>(row.size()), row.data());
    for (int x = 0; x < static_cast<int>(row.size()); ++x) {
      CHECK(row[static_cast<std::size_t>(x)] == shape.coverage(x, y));
    }
  }
}

TEST_CASE("roundedCoverage is whole away from the corners and matches the sampled shape in them") {
  const RoundedRect shape{.width = 40, .height = 20, .corners = kRound8};

  CHECK(roundedCoverage(kRound8, 40, 20, 20, 0) == 1.0f);
  CHECK(roundedCoverage(kRound8, 40, 20, 0, 10) == 1.0f);
  CHECK(roundedCoverage(kRound8, 40, 20, 1, 1) == shape.coverage(1, 1));
  CHECK(roundedCoverage(kRound8, 40, 20, 38, 18) == shape.coverage(38, 18));
}

} // namespace rntizen
