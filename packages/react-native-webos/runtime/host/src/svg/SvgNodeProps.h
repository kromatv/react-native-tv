#pragma once

#include "SvgFont.h"
#include "SvgNode.h"
#include "SvgTransform.h"

namespace rnwebos::svg {

/// Reads what every react-native-svg element carries (`SvgNodeCommonProps`).
template <typename Props>
void readNodeProps(Node &node, const Props &props) {
  node.name = props.name;
  node.matrix = matrixFromProp(props.matrix);
  node.opacity = props.opacity;
  node.hidden = props.display == "none";
  node.clipPath = props.clipPath;
  node.clipRule = props.clipRule;
  node.mask = props.mask;
}

/// Reads the presentation attributes of a drawn element (`SvgRenderableCommonProps`).
template <typename Props>
void readRenderableProps(Node &node, const Props &props) {
  readNodeProps(node, props);
  node.color = colorFromProp(props.color);
  node.paint = {
      .fill = brushFromProp(props.fill),
      .fillOpacity = props.fillOpacity,
      .fillRule = props.fillRule,
      .stroke = brushFromProp(props.stroke),
      .strokeOpacity = props.strokeOpacity,
      .strokeLinecap = props.strokeLinecap,
      .strokeLinejoin = props.strokeLinejoin,
      .strokeDasharray = lengthsFromProp(props.strokeDasharray),
      .strokeDashoffset = props.strokeDashoffset,
      .listed = paintAttributesFromPropList(props.propList),
  };
  if (const auto width = lengthFromProp(props.strokeWidth)) node.paint.strokeWidth = *width;
  if (props.strokeMiterlimit > 0) node.paint.strokeMiterlimit = props.strokeMiterlimit;
}

inline constexpr Length kNoPercent{.value = 0, .percent = true};
inline constexpr Length kHalf{.value = 50, .percent = true};
inline constexpr Length kWhole{.value = 100, .percent = true};

inline Length lengthOr(const folly::dynamic &value, Length fallback = {}) {
  return lengthFromProp(value).value_or(fallback);
}

} // namespace rnwebos::svg
