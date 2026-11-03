#include "GradientRamp.h"

#include <doctest/doctest.h>

#include <vector>

namespace rntizen {

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

void checkColor(const Premultiplied &actual, const Premultiplied &expected) {
  for (std::size_t c = 0; c < actual.size(); ++c) CHECK(actual[c] == doctest::Approx(expected[c]).epsilon(1e-4));
}

const auto kRed = colorFromRGBA(255, 0, 0, 255);
const auto kBlue = colorFromRGBA(0, 0, 255, 255);
const auto kClear = colorFromRGBA(0, 0, 1, 0);

} // namespace

TEST_CASE("GradientRamp spreads unpositioned stops evenly from 0 to 1") {
  const GradientRamp ramp({stop(kRed), stop(kBlue)}, 100, 1);

  checkColor(ramp.at(0), {1, 0, 0, 1});
  checkColor(ramp.at(0.5f), {0.5f, 0, 0.5f, 1});
  checkColor(ramp.at(1), {0, 0, 1, 1});
}

TEST_CASE("GradientRamp holds the end colors beyond the line") {
  const GradientRamp ramp({stop(kRed, percent(25)), stop(kBlue, percent(75))}, 100, 1);

  checkColor(ramp.at(-1), {1, 0, 0, 1});
  checkColor(ramp.at(0.25f), {1, 0, 0, 1});
  checkColor(ramp.at(0.5f), {0.5f, 0, 0.5f, 1});
  checkColor(ramp.at(2), {0, 0, 1, 1});
}

TEST_CASE("GradientRamp interpolates premultiplied, so a clear stop fades without darkening") {
  const GradientRamp ramp({stop(kRed), stop(kClear)}, 100, 1);

  checkColor(ramp.at(0.5f), {0.5f, 0, 0, 0.5f});
}

TEST_CASE("GradientRamp measures point positions along the line at the point scale") {
  const GradientRamp ramp({stop(kRed, {10, UnitType::Point}), stop(kBlue, {30, UnitType::Point})}, 80, 2);

  checkColor(ramp.at(0.25f), {1, 0, 0, 1});
  checkColor(ramp.at(0.5f), {0.5f, 0, 0.5f, 1});
  checkColor(ramp.at(0.75f), {0, 0, 1, 1});
}

TEST_CASE("GradientRamp moves a stop placed before an earlier one up to it") {
  const GradientRamp ramp({stop(kRed, percent(60)), stop(kBlue, percent(20))}, 100, 1);

  checkColor(ramp.at(0.59f), {1, 0, 0, 1});
  checkColor(ramp.at(0.61f), {0, 0, 1, 1});
}

TEST_CASE("GradientRamp bends the transition through a hint") {
  const GradientRamp ramp({stop(kRed, percent(0)), stop({}, percent(25)), stop(kBlue, percent(100))}, 100, 1);

  checkColor(ramp.at(0.25f), {0.5f, 0, 0.5f, 1});
}

TEST_CASE("GradientRamp with a segment hint gives the colors a fresh search gives") {
  const GradientRamp ramp(
      {stop(kRed), stop(kBlue), stop(colorFromRGBA(0, 255, 0, 255)), stop(kClear)}, 300, 1);
  std::size_t segment = 0;

  for (int i = 0; i <= 100; ++i) {
    const auto t = static_cast<float>(i) / 100;
    const auto walked = ramp.at(t, segment);
    const auto searched = ramp.at(t);
    for (std::size_t c = 0; c < walked.size(); ++c) CHECK(walked[c] == searched[c]);
  }
}

TEST_CASE("GradientRamp without a color is empty") {
  CHECK(GradientRamp({}, 100, 1).empty());
  CHECK_FALSE(GradientRamp({stop(kRed)}, 100, 1).empty());
}

} // namespace rntizen
