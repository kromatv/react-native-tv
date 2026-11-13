#include "BoxBorder.h"

#include "ViewFixtures.test.h"

#include <doctest/doctest.h>

#include <cmath>
#include <cstdint>
#include <cstdlib>

namespace rntizen {

namespace {

using facebook::react::colorFromRGBA;
using facebook::react::SharedColor;
using testing::isNear;
using testing::kBlack;
using testing::kRed;

constexpr std::uint32_t kGreen = 0xff00ff00;
constexpr std::uint32_t kBlue = 0xff0000ff;
constexpr std::uint32_t kYellow = 0xffffff00;
constexpr int kSize = 100;

SharedColor colorOf(std::uint32_t argb) {
  return colorFromRGBA(argb >> 16 & 0xff, argb >> 8 & 0xff, argb & 0xff, argb >> 24);
}

struct Sides {
  std::uint32_t top{0};
  std::uint32_t right{0};
  std::uint32_t bottom{0};
  std::uint32_t left{0};
};

/// Whether `pixel` is some mix of `a` and `b` and nothing else, so nothing
/// behind the border shows through where two sides meet.
bool isMixOf(std::uint32_t pixel, std::uint32_t a, std::uint32_t b) {
  const auto channel = [](std::uint32_t color, int shift) { return static_cast<int>(color >> shift & 0xff); };
  int widest = 0;
  for (int shift = 8; shift < 32; shift += 8) {
    if (std::abs(channel(a, shift) - channel(b, shift)) > std::abs(channel(a, widest) - channel(b, widest))) widest = shift;
  }
  const auto share = static_cast<float>(channel(pixel, widest) - channel(b, widest)) /
      static_cast<float>(channel(a, widest) - channel(b, widest));
  if (share < 0 || share > 1) return false;
  for (int shift = 0; shift < 32; shift += 8) {
    const auto mixed = share * static_cast<float>(channel(a, shift)) + (1 - share) * static_cast<float>(channel(b, shift));
    if (std::abs(static_cast<float>(channel(pixel, shift)) - mixed) > 4) return false;
  }
  return true;
}

/// A black screen with one bordered box at its origin.
struct Stage {
  Stage(
      float radius,
      const Edges &widths,
      const Sides &colors,
      facebook::react::BorderStyle style = facebook::react::BorderStyle::Solid,
      int size = kSize) {
    auto frame = testing::frameOf(0, 0, size, size);
    frame.borderWidth = {
        .left = static_cast<facebook::react::Float>(widths.left),
        .top = static_cast<facebook::react::Float>(widths.top),
        .right = static_cast<facebook::react::Float>(widths.right),
        .bottom = static_cast<facebook::react::Float>(widths.bottom),
    };
    const auto props = testing::propsWith([&](facebook::react::ViewProps &view) {
      view.borderRadii.all = facebook::react::ValueUnit{radius, facebook::react::UnitType::Point};
      view.borderStyles.all = style;
      view.borderColors.top = colorOf(colors.top);
      view.borderColors.right = colorOf(colors.right);
      view.borderColors.bottom = colorOf(colors.bottom);
      view.borderColors.left = colorOf(colors.left);
    });
    testing::place(root, testing::filled(0, 0, 0), testing::frameOf(0, 0, kSize, kSize));
    testing::place(box, props, frame);
    root.mountChild(box, 0);
    root.mountAsRoot();
  }

  std::uint32_t pixel(int x, int y) const {
    return canvas.pixel(x, y);
  }

  const testing::TestCanvas canvas{kSize};
  ComponentView root{canvas.evas(), 1};
  ComponentView box{canvas.evas(), 2};
};

constexpr Edges kEven{.left = 10, .top = 10, .right = 10, .bottom = 10};

} // namespace

TEST_CASE("a circle with two colored and two transparent sides draws half a ring, from one diagonal to the other") {
  const Stage stage(50, kEven, {.top = kRed, .right = kRed});

  CHECK(isNear(stage.pixel(50, 5), kRed));
  CHECK(isNear(stage.pixel(95, 50), kRed));
  CHECK(isNear(stage.pixel(27, 11), kRed));
  CHECK(isNear(stage.pixel(88, 72), kRed));
  CHECK(isNear(stage.pixel(82, 17), kRed));
  CHECK(stage.pixel(11, 27) == kBlack);
  CHECK(stage.pixel(72, 88) == kBlack);
  CHECK(stage.pixel(50, 95) == kBlack);
  CHECK(stage.pixel(5, 50) == kBlack);
  CHECK(stage.pixel(50, 50) == kBlack);
  CHECK(stage.pixel(50, 12) == kBlack);
}

TEST_CASE("a circle too small for its corner pieces still draws its half ring") {
  const Edges thin{.left = 2, .top = 2, .right = 2, .bottom = 2};
  const Stage stage(9.5f, thin, {.top = kRed, .right = kRed}, facebook::react::BorderStyle::Solid, 19);

  CHECK(isNear(stage.pixel(9, 0), kRed));
  CHECK(isNear(stage.pixel(9, 1), kRed));
  CHECK(isNear(stage.pixel(18, 9), kRed));
  CHECK(stage.pixel(9, 3) == kBlack);
  CHECK(stage.pixel(9, 18) == kBlack);
  CHECK(stage.pixel(0, 9) == kBlack);
  CHECK(stage.pixel(9, 9) == kBlack);
}

TEST_CASE("four colors on a circle each keep their side, and meet with nothing showing through") {
  const Stage stage(50, kEven, {.top = kRed, .right = kGreen, .bottom = kBlue, .left = kYellow});

  CHECK(isNear(stage.pixel(50, 5), kRed));
  CHECK(isNear(stage.pixel(27, 11), kRed));
  CHECK(isNear(stage.pixel(72, 11), kRed));
  CHECK(isNear(stage.pixel(95, 50), kGreen));
  CHECK(isNear(stage.pixel(88, 27), kGreen));
  CHECK(isNear(stage.pixel(88, 72), kGreen));
  CHECK(isNear(stage.pixel(50, 95), kBlue));
  CHECK(isNear(stage.pixel(72, 88), kBlue));
  CHECK(isNear(stage.pixel(27, 88), kBlue));
  CHECK(isNear(stage.pixel(5, 50), kYellow));
  CHECK(isNear(stage.pixel(11, 72), kYellow));
  CHECK(isNear(stage.pixel(11, 27), kYellow));
  CHECK(stage.pixel(50, 50) == kBlack);
  for (int i = 16; i <= 20; ++i) {
    CAPTURE(i);
    CHECK(isMixOf(stage.pixel(i, i), kRed, kYellow));
    CHECK(isMixOf(stage.pixel(kSize - 1 - i, i), kRed, kGreen));
    CHECK(isMixOf(stage.pixel(kSize - 1 - i, kSize - 1 - i), kGreen, kBlue));
    CHECK(isMixOf(stage.pixel(i, kSize - 1 - i), kBlue, kYellow));
  }
}

TEST_CASE("sides of two widths meet on the line from the box's corner through the padding box's") {
  const Stage stage(30, {.left = 5, .top = 20, .right = 5, .bottom = 5}, {.top = kRed, .right = kBlue, .bottom = kBlue, .left = kBlue});

  CHECK(isNear(stage.pixel(50, 10), kRed));
  CHECK(stage.pixel(50, 22) == kBlack);
  CHECK(isNear(stage.pixel(2, 50), kBlue));
  CHECK(stage.pixel(8, 50) == kBlack);
  CHECK(isNear(stage.pixel(9, 22), kRed));
  CHECK(isNear(stage.pixel(3, 26), kBlue));
  CHECK(isNear(stage.pixel(90, 22), kRed));
  CHECK(isNear(stage.pixel(96, 26), kBlue));
  CHECK(isNear(stage.pixel(50, 97), kBlue));
}

TEST_CASE("one color on a rounded box draws the whole ring as before") {
  const Stage stage(50, kEven, {.top = kRed, .right = kRed, .bottom = kRed, .left = kRed});

  CHECK(isNear(stage.pixel(50, 5), kRed));
  CHECK(isNear(stage.pixel(95, 50), kRed));
  CHECK(isNear(stage.pixel(50, 95), kRed));
  CHECK(isNear(stage.pixel(5, 50), kRed));
  CHECK(isNear(stage.pixel(17, 17), kRed));
  CHECK(isNear(stage.pixel(82, 82), kRed));
  CHECK(stage.pixel(50, 50) == kBlack);
  CHECK(stage.pixel(2, 2) == kBlack);
}

TEST_CASE("a dashed border with a color per side cuts each side's dashes in its own color") {
  const Stage stage(20, {.left = 6, .top = 6, .right = 6, .bottom = 6}, {.top = kRed, .left = kBlue}, facebook::react::BorderStyle::Dashed);
  int red = 0;
  int gaps = 0;
  int blue = 0;
  int clear = 0;
  for (int i = 25; i < 75; ++i) {
    red += isNear(stage.pixel(i, 3), kRed) ? 1 : 0;
    gaps += stage.pixel(i, 3) == kBlack ? 1 : 0;
    blue += isNear(stage.pixel(3, i), kBlue) ? 1 : 0;
    clear += stage.pixel(kSize - 4, i) == kBlack && stage.pixel(i, kSize - 4) == kBlack ? 1 : 0;
  }

  CHECK(red > 10);
  CHECK(gaps > 5);
  CHECK(blue > 10);
  CHECK(clear == 50);
}

} // namespace rntizen
