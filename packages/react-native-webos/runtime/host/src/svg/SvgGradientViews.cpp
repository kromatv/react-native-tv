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
    auto definition = commonDefinition(linear);
    definition.x1 = lengthOr(linear.x1, {.value = 0, .percent = true});
    definition.y1 = lengthOr(linear.y1, {.value = 0, .percent = true});
    definition.x2 = lengthOr(linear.x2, {.value = 100, .percent = true});
    definition.y2 = lengthOr(linear.y2, {.value = 0, .percent = true});
    setGradient(linear.name, definition);
  }
};

class SvgRadialGradientView final : public SvgGradientView {
 public:
  using SvgGradientView::SvgGradientView;

 protected:
  void readProps(const facebook::react::Props &props) override {
    const auto &radial = static_cast<const facebook::react::RNSVGRadialGradientProps &>(props);
    constexpr Length kHalf{.value = 50, .percent = true};
    auto definition = commonDefinition(radial);
    definition.radial = true;
    definition.cx = lengthOr(radial.cx, kHalf);
    definition.cy = lengthOr(radial.cy, kHalf);
    definition.rx = lengthOr(radial.rx, kHalf);
    definition.ry = lengthOr(radial.ry, kHalf);
    definition.fx = lengthOr(radial.fx, definition.cx);
    definition.fy = lengthOr(radial.fy, definition.cy);
    setGradient(radial.name, definition);
  }
};

} // namespace

const std::string &SvgGradientView::name() const {
  return name_;
}

const GradientDefinition &SvgGradientView::definition() const {
  return definition_;
}

void SvgGradientView::drawContent(Drawing &, const DrawContext &) const {
  // A gradient paints only through the shapes that reference it by id.
}

void SvgGradientView::setGradient(std::string_view name, const GradientDefinition &definition) {
  name_ = name;
  definition_ = definition;
}

void collectGradients(std::span<ComponentView *const> views, GradientDefinitions &gradients) {
  for (const auto *view : views) {
    if (view == nullptr) continue;
    if (const auto *gradient = dynamic_cast<const SvgGradientView *>(view); gradient != nullptr && !gradient->name().empty()) {
      gradients.try_emplace(gradient->name(), gradient->definition());
    }
    collectGradients(view->children(), gradients);
  }
}

RN_TIZEN_COMPONENT_VIEW("RNSVGLinearGradient", SvgLinearGradientView);
RN_TIZEN_COMPONENT_VIEW("RNSVGRadialGradient", SvgRadialGradientView);

} // namespace rntizen::svg
