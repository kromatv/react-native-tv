#pragma once

#include "SvgGradient.h"
#include "SvgPaint.h"
#include "SvgPath.h"

#include <optional>
#include <vector>

namespace rntizen::svg {

/// Where a node draws: the transform to the `<Svg>` view's pixels, the opacity
/// and paint it inherits, the color `currentColor` stands for, the size
/// percentages resolve against (the viewBox, else the view), and the gradients
/// a `url(#id)` paint names.
struct DrawContext {
  Affine transform;
  double opacity{1};
  Paint paint;
  Rgba currentColor;
  double viewportWidth{0};
  double viewportHeight{0};
  const GradientDefinitions *gradients{nullptr};

  double viewportDiagonal() const;
};

/// One shape ready for Evas VG: its path in the view's pixels, and its colors
/// or gradients with every opacity on the way down already applied.
struct DrawnShape {
  Path path;
  std::optional<Rgba> fill;
  std::optional<Rgba> stroke;
  std::optional<GradientPaint> fillGradient;
  std::optional<GradientPaint> strokeGradient;
  double strokeWidth{0};
  int strokeLinecap{0};
  int strokeLinejoin{0};
};

using Drawing = std::vector<DrawnShape>;

/// What every react-native-svg node carries besides its paint.
struct NodeAttributes {
  double opacity{1};
  Affine matrix;
  bool hidden{false};
  std::optional<Rgba> color;
};

template <typename Props>
NodeAttributes readNodeAttributes(const Props &props) {
  NodeAttributes node{.opacity = props.opacity, .hidden = props.display == "none", .color = rgbaFromColor(props.color)};
  if (props.matrix.size() == 6) {
    node.matrix = {
        .a = props.matrix[0],
        .b = props.matrix[1],
        .c = props.matrix[2],
        .d = props.matrix[3],
        .e = props.matrix[4],
        .f = props.matrix[5],
    };
  }
  return node;
}

template <typename Props>
Paint readPaint(const Props &props) {
  Paint paint{
      .fill = brushFromProp(props.fill),
      .fillOpacity = props.fillOpacity,
      .fillRule = props.fillRule,
      .stroke = brushFromProp(props.stroke),
      .strokeOpacity = props.strokeOpacity,
      .strokeLinecap = props.strokeLinecap,
      .strokeLinejoin = props.strokeLinejoin,
      .listed = paintAttributesFromPropList(props.propList),
  };
  if (const auto width = lengthFromProp(props.strokeWidth)) paint.strokeWidth = *width;
  if (props.strokeMiterlimit > 0) paint.strokeMiterlimit = props.strokeMiterlimit;
  return paint;
}

/// Turns a shape's path, in its own user units, into what Evas VG draws.
DrawnShape drawnShape(const Path &path, const DrawContext &context);

} // namespace rntizen::svg
