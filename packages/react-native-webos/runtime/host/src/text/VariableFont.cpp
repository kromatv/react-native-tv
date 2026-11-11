#include "VariableFont.h"

#include "include/core/SkFontArguments.h"
#include "include/core/SkFontParameters.h"

#include <algorithm>
#include <utility>

namespace rnwebos {

namespace {

using Axis = SkFontParameters::Variation::Axis;
using Coordinate = SkFontArguments::VariationPosition::Coordinate;

constexpr int kLightestWeight = 100;
constexpr int kHeaviestWeight = 900;
constexpr int kWeightStep = 100;

} // namespace

std::vector<sk_sp<SkTypeface>> weightInstances(sk_sp<SkTypeface> typeface) {
  const int axisCount = typeface->getVariationDesignParameters({});
  if (axisCount <= 0) return {std::move(typeface)};
  std::vector<Axis> axes(static_cast<std::size_t>(axisCount));
  std::vector<Coordinate> position(axes.size());
  if (typeface->getVariationDesignParameters(axes) != axisCount ||
      typeface->getVariationDesignPosition(position) != axisCount) {
    return {std::move(typeface)};
  }
  const auto weightAxis = std::ranges::find(axes, Coordinate::wght, &Axis::tag);
  if (weightAxis == axes.end()) return {std::move(typeface)};
  std::vector<sk_sp<SkTypeface>> instances;
  for (int weight = kLightestWeight; weight <= kHeaviestWeight; weight += kWeightStep) {
    if (static_cast<float>(weight) < weightAxis->min || static_cast<float>(weight) > weightAxis->max) continue;
    for (auto &coordinate : position) {
      if (coordinate.axis == Coordinate::wght) coordinate.value = static_cast<float>(weight);
    }
    SkFontArguments arguments;
    arguments.setVariationDesignPosition({position.data(), axisCount});
    if (auto instance = typeface->makeClone(arguments)) instances.push_back(std::move(instance));
  }
  if (instances.empty()) instances.push_back(std::move(typeface));
  return instances;
}

} // namespace rnwebos
