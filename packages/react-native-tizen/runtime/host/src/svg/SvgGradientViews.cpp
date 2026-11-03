#include "../mounting/ComponentViewRegistry.h"
#include "SvgGradientView.h"

#include <react/renderer/components/rnsvg/Props.h>

namespace rntizen::svg {

namespace {

constexpr int kUserSpaceOnUse = 1;

Length lengthOr(const folly::dynamic &value, Length fallback) {
  return lengthFromProp(value).value_or(fallback);
}

Affine affineOf(const std::vector<facebook::react::Float> &matrix) {
  if (matrix.size() != 6) return {};
  return {.a = matrix[0], .b = matrix[1], .c = matrix[2], .d = matrix[3], .e = matrix[4], .f = matrix[5]};
}

template <typename Props>
GradientDefinition commonDefinition(const Props &props) {
  return {
      .stops = gradientStops(props.gradient),
      .userSpace = props.gradientUnits == kUserSpaceOnUse,
      .transform = affineOf(props.gradientTransform),
  };
}

class SvgLinearGradientView final : public SvgGradientView {
 public:
  using SvgGradientView::SvgGradientView;

 protected:
  void readProps(const facebook::react::Props &props) override {
    const auto &linear = static_cast<const facebook::react::RNSVGLinearGradientProps &>(props);
    name_ = linear.name;
    definition_ = commonDefinition(linear);
    definition_.x1 = lengthOr(linear.x1, {.value = 0, .percent = true});
    definition_.y1 = lengthOr(linear.y1, {.value = 0, .percent = true});
    definition_.x2 = lengthOr(linear.x2, {.value = 100, .percent = true});
    definition_.y2 = lengthOr(linear.y2, {.value = 0, .percent = true});
  }
};

class SvgRadialGradientView final : public SvgGradientView {
 public:
  using SvgGradientView::SvgGradientView;

 protected:
  void readProps(const facebook::react::Props &props) override {
    const auto &radial = static_cast<const facebook::react::RNSVGRadialGradientProps &>(props);
    constexpr Length kHalf{.value = 50, .percent = true};
    name_ = radial.name;
    definition_ = commonDefinition(radial);
    definition_.radial = true;
    definition_.cx = lengthOr(radial.cx, kHalf);
    definition_.cy = lengthOr(radial.cy, kHalf);
    definition_.rx = lengthOr(radial.rx, kHalf);
    definition_.ry = lengthOr(radial.ry, kHalf);
    definition_.fx = lengthOr(radial.fx, definition_.cx);
    definition_.fy = lengthOr(radial.fy, definition_.cy);
  }
};

} // namespace

const std::string &SvgGradientView::name() const {
  return name_;
}

const GradientDefinition &SvgGradientView::definition() const {
  return definition_;
}

void SvgGradientView::drawContent(Drawing &, const DrawContext &) const {}

void collectGradients(const std::vector<ComponentView *> &views, GradientDefinitions &gradients) {
  for (const auto *view : views) {
    if (const auto *gradient = dynamic_cast<const SvgGradientView *>(view); gradient != nullptr && !gradient->name().empty()) {
      gradients.emplace(gradient->name(), gradient->definition());
    }
    collectGradients(view->children(), gradients);
  }
}

RN_TIZEN_COMPONENT_VIEW("RNSVGLinearGradient", SvgLinearGradientView);
RN_TIZEN_COMPONENT_VIEW("RNSVGRadialGradient", SvgRadialGradientView);

} // namespace rntizen::svg
