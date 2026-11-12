#include "GradientRamp.h"

#include <doctest/doctest.h>

#include <array>
#include <cmath>
#include <vector>

namespace rnwebos {

namespace {

using facebook::react::ColorStop;
using facebook::react::colorFromRGBA;
using facebook::react::UnitType;
using facebook::react::ValueUnit;

ColorStop stop(facebook::react::SharedColor color, ValueUnit position = {}) {
  return {.color = color, .position = position};
}

ValueUnit percent(float value) {
  return {value, UnitType::Percent};
}

// `color` premultiplied, the form a gradient interpolates in.
std::array<float, 4> premultiplied(const SkColor4f &color) {
  return {color.fR * color.fA, color.fG * color.fA, color.fB * color.fA, color.fA};
}

void checkPremultiplied(const std::array<float, 4> &actual, const std::array<float, 4> &expected) {
  for (std::size_t c = 0; c < actual.size(); ++c) CHECK(actual[c] == doctest::Approx(expected[c]).epsilon(1e-4));
}

void checkColor(const SkColor4f &actual, const std::array<float, 4> &expected) {
  checkPremultiplied(premultiplied(actual), expected);
}

// The premultiplied color straight lines between the ramp's stops give at `t`.
std::array<float, 4> drawnAt(const GradientRamp &ramp, float t) {
  const auto &positions = ramp.positions();
  const auto &colors = ramp.colors();
  if (t <= positions.front()) return premultiplied(colors.front());
  if (t >= positions.back()) return premultiplied(colors.back());
  std::size_t upper = 1;
  while (positions[upper] < t) ++upper;
  const auto from = premultiplied(colors[upper - 1]);
  const auto to = premultiplied(colors[upper]);
  const auto weight = (t - positions[upper - 1]) / (positions[upper] - positions[upper - 1]);
  std::array<float, 4> color{};
  for (std::size_t c = 0; c < color.size(); ++c) color[c] = from[c] + (to[c] - from[c]) * weight;
  return color;
}

const auto kRed = colorFromRGBA(255, 0, 0, 255);
const auto kBlue = colorFromRGBA(0, 0, 255, 255);
const auto kClear = colorFromRGBA(0, 0, 1, 0);

} // namespace

TEST_CASE("GradientRamp spreads unpositioned stops evenly from 0 to 1") {
  const GradientRamp ramp({stop(kRed), stop(colorFromRGBA(0, 255, 0, 255)), stop(kBlue)}, 100, 1);

  CHECK(ramp.positions() == std::vector<float>{0, 0.5f, 1});
  checkColor(ramp.colors()[0], {1, 0, 0, 1});
  checkColor(ramp.colors()[1], {0, 1, 0, 1});
  checkColor(ramp.colors()[2], {0, 0, 1, 1});
}

TEST_CASE("GradientRamp keeps positions before 0 and after 1 as CSS allows them") {
  const GradientRamp ramp({stop(kRed, percent(-50)), stop(kBlue, percent(150))}, 100, 1);

  CHECK(ramp.positions() == std::vector<float>{-0.5f, 1.5f});
}

TEST_CASE("GradientRamp keeps a clear stop's alpha at zero, so it fades without darkening") {
  const GradientRamp ramp({stop(kRed), stop(kClear)}, 100, 1);

  checkColor(ramp.colors()[1], {0, 0, 0, 0});
  checkPremultiplied(drawnAt(ramp, 0.5f), {0.5f, 0, 0, 0.5f});
}

TEST_CASE("GradientRamp measures point positions along the line at the point scale") {
  const GradientRamp ramp({stop(kRed, {10, UnitType::Point}), stop(kBlue, {30, UnitType::Point})}, 80, 2);

  CHECK(ramp.positions() == std::vector<float>{0.25f, 0.75f});
}

TEST_CASE("GradientRamp moves a stop placed before an earlier one up to it") {
  const GradientRamp ramp({stop(kRed, percent(60)), stop(kBlue, percent(20))}, 100, 1);

  CHECK(ramp.positions() == std::vector<float>{0.6f, 0.6f});
}

TEST_CASE("GradientRamp follows a hint's curve within half a color level") {
  for (const auto hint : {20.0f, 35.0f, 65.0f, 80.0f}) {
    CAPTURE(hint);
    const GradientRamp ramp({stop(kRed, percent(0)), stop({}, percent(hint)), stop(kBlue, percent(100))}, 100, 1);
    const auto exponent = std::log(0.5f) / std::log(hint / 100);

    for (int i = 4; i <= 1000; ++i) {
      const auto t = static_cast<float>(i) / 1000;
      const auto blue = std::pow(t, exponent);
      CHECK(std::abs(drawnAt(ramp, t)[2] - blue) < 0.6f / 255);
      CHECK(std::abs(drawnAt(ramp, t)[0] - (1 - blue)) < 0.6f / 255);
    }
    CHECK(drawnAt(ramp, hint / 100)[2] == doctest::Approx(0.5f).epsilon(0.01));
    CHECK(ramp.colors().size() <= 34);
  }
}

TEST_CASE("GradientRamp turns a hint at either end of its transition into a hard stop") {
  const GradientRamp early({stop(kRed, percent(20)), stop({}, percent(20)), stop(kBlue, percent(80))}, 100, 1);
  const GradientRamp late({stop(kRed, percent(20)), stop({}, percent(80)), stop(kBlue, percent(80))}, 100, 1);

  CHECK(early.positions() == std::vector<float>{0.2f, 0.2f, 0.8f});
  checkColor(early.colors()[1], {0, 0, 1, 1});
  CHECK(late.positions() == std::vector<float>{0.2f, 0.8f, 0.8f});
  checkColor(late.colors()[1], {1, 0, 0, 1});
}

TEST_CASE("GradientRamp ignores a hint with no color on one side") {
  const GradientRamp ramp({stop({}, percent(10)), stop(kRed), stop(kBlue), stop({}, percent(90))}, 100, 1);

  CHECK(ramp.colors().size() == 2);
}

TEST_CASE("GradientRamp without a color is empty") {
  CHECK(GradientRamp({}, 100, 1).empty());
  CHECK_FALSE(GradientRamp({stop(kRed)}, 100, 1).empty());
}

TEST_CASE("between cuts the ramp to a part of the line and measures positions along that part") {
  const GradientRamp ramp({stop(kRed, percent(-50)), stop(kBlue, percent(150))}, 100, 1);

  const auto part = ramp.between(0, 1);

  CHECK(part.positions() == std::vector<float>{0, 1});
  checkColor(part.colors()[0], {0.75f, 0, 0.25f, 1});
  checkColor(part.colors()[1], {0.25f, 0, 0.75f, 1});
}

TEST_CASE("between holds the end colors of a ramp shorter than the part") {
  const GradientRamp ramp({stop(kRed, percent(25)), stop(kBlue, percent(75))}, 100, 1);

  const auto part = ramp.between(0, 1);

  CHECK(part.positions() == std::vector<float>{0, 0.25f, 0.75f, 1});
  checkColor(part.colors().front(), {1, 0, 0, 1});
  checkColor(part.colors().back(), {0, 0, 1, 1});
}

TEST_CASE("between keeps a hard stop that sits on the part's end, so the color past it holds") {
  const GradientRamp ramp({stop(kRed, percent(0)), stop(kRed, percent(50)), stop(kBlue, percent(50))}, 100, 1);

  const auto part = ramp.between(0, 0.5f);

  CHECK(part.positions() == std::vector<float>{0, 1, 1});
  checkColor(part.colors()[1], {1, 0, 0, 1});
  checkColor(part.colors()[2], {0, 0, 1, 1});
}

TEST_CASE("between stretches a longer part's positions and gives one color when every stop is past it") {
  const GradientRamp ramp({stop(kRed, percent(0)), stop(kBlue, percent(200))}, 100, 1);
  const GradientRamp far({stop(kRed, percent(150)), stop(kBlue, percent(160))}, 100, 1);

  CHECK(ramp.between(0, 2).positions() == std::vector<float>{0, 1});
  const auto part = far.between(0, 1);
  CHECK(part.positions() == std::vector<float>{0, 1});
  checkColor(part.colors()[0], {1, 0, 0, 1});
  checkColor(part.colors()[1], {1, 0, 0, 1});
}

} // namespace rnwebos
