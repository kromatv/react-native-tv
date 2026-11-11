#include "VariableFont.h"

#include "include/core/SkFontMgr.h"
#include "include/core/SkFontStyle.h"
#include "include/ports/SkFontMgr_directory.h"

#include <doctest/doctest.h>

#include <string>
#include <vector>

namespace rnwebos {

namespace {

sk_sp<SkTypeface> skiaTestFont(const char *name) {
  static const auto fonts = SkFontMgr_New_Custom_Directory(RN_WEBOS_TEST_FONT_DIR);
  return fonts->makeFromFile((std::string(RN_WEBOS_TEST_FONT_DIR) + "/" + name).c_str(), 0);
}

std::vector<int> weightsOf(const std::vector<sk_sp<SkTypeface>> &typefaces) {
  std::vector<int> weights;
  for (const auto &typeface : typefaces) weights.push_back(typeface->fontStyle().weight());
  return weights;
}

} // namespace

TEST_CASE("a variable font with a weight axis gives one instance per hundred, each reporting its weight") {
  auto variable = skiaTestFont("Variable.ttf");
  REQUIRE(variable != nullptr);
  CHECK(variable->fontStyle().weight() == SkFontStyle::kNormal_Weight);

  const auto instances = weightInstances(variable);

  CHECK(weightsOf(instances) == std::vector<int>{100, 200, 300, 400, 500, 600, 700, 800, 900});
  for (const auto &instance : instances) {
    std::vector<SkFontArguments::VariationPosition::Coordinate> position(2);
    REQUIRE(instance->getVariationDesignPosition(position) == 2);
    for (const auto &coordinate : position) {
      if (coordinate.axis == SkFontArguments::VariationPosition::Coordinate::wght) {
        CHECK(coordinate.value == static_cast<float>(instance->fontStyle().weight()));
      } else {
        CHECK(coordinate.value == 100.0f);
      }
    }
  }
}

TEST_CASE("a static font, and a variable one whose weight axis covers no hundred, stay as they are") {
  auto fixed = skiaTestFont("Roboto-Regular.ttf");
  REQUIRE(fixed != nullptr);
  const auto fromFixed = weightInstances(fixed);
  REQUIRE(fromFixed.size() == 1);
  CHECK(fromFixed[0] == fixed);

  auto distortable = skiaTestFont("Distortable.ttf");
  REQUIRE(distortable != nullptr);
  const auto fromDistortable = weightInstances(distortable);
  REQUIRE(fromDistortable.size() == 1);
  CHECK(fromDistortable[0] == distortable);
}

} // namespace rnwebos
