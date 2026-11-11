#include "../mounting/ComponentViewRegistry.h"
#include "SvgGradientNode.h"
#include "SvgNodeProps.h"
#include "SvgNodeView.h"

#include <react/renderer/components/rnsvg/Props.h>

namespace rnwebos::svg {

namespace {

namespace react = facebook::react;

constexpr int kUserSpaceOnUse = 1;

template <typename Props>
GradientDefinition commonDefinition(const Props &props) {
  return {
      .stops = gradientStops(props.gradient),
      .userSpace = props.gradientUnits == kUserSpaceOnUse,
      .transform = matrixFromProp(props.gradientTransform),
  };
}

void readLinearGradient(GradientNode &node, const react::RNSVGLinearGradientProps &props) {
  readNodeProps(node, props);
  node.definition = commonDefinition(props);
  node.definition.x1 = lengthOr(props.x1, kNoPercent);
  node.definition.y1 = lengthOr(props.y1, kNoPercent);
  node.definition.x2 = lengthOr(props.x2, kWhole);
  node.definition.y2 = lengthOr(props.y2, kNoPercent);
}

void readRadialGradient(GradientNode &node, const react::RNSVGRadialGradientProps &props) {
  readNodeProps(node, props);
  auto &definition = node.definition;
  definition = commonDefinition(props);
  definition.radial = true;
  definition.cx = lengthOr(props.cx, kHalf);
  definition.cy = lengthOr(props.cy, kHalf);
  definition.rx = lengthOr(props.rx, kHalf);
  definition.ry = lengthOr(props.ry, kHalf);
  definition.fx = lengthOr(props.fx, definition.cx);
  definition.fy = lengthOr(props.fy, definition.cy);
}

using SvgLinearGradientView = SvgNodeViewOf<GradientNode, react::RNSVGLinearGradientProps, readLinearGradient>;
using SvgRadialGradientView = SvgNodeViewOf<GradientNode, react::RNSVGRadialGradientProps, readRadialGradient>;

} // namespace

RN_WEBOS_COMPONENT_VIEW("RNSVGLinearGradient", SvgLinearGradientView);
RN_WEBOS_COMPONENT_VIEW("RNSVGRadialGradient", SvgRadialGradientView);

} // namespace rnwebos::svg
