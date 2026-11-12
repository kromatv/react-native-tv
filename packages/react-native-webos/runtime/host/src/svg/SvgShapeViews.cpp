#include "../mounting/ComponentViewRegistry.h"
#include "SvgNodeProps.h"
#include "SvgNodeView.h"
#include "SvgShapeNode.h"

#include <react/renderer/components/rnsvg/Props.h>

namespace rnwebos::svg {

namespace {

namespace react = facebook::react;

void readPath(PathNode &node, const react::RNSVGPathProps &props) {
  readRenderableProps(node, props);
  node.setGeometry({.data = props.d});
}

void readCircle(CircleNode &node, const react::RNSVGCircleProps &props) {
  readRenderableProps(node, props);
  node.setGeometry({.cx = lengthOr(props.cx), .cy = lengthOr(props.cy), .r = lengthOr(props.r)});
}

void readEllipse(EllipseNode &node, const react::RNSVGEllipseProps &props) {
  readRenderableProps(node, props);
  node.setGeometry(
      {.cx = lengthOr(props.cx), .cy = lengthOr(props.cy), .rx = lengthOr(props.rx), .ry = lengthOr(props.ry)});
}

void readRect(RectNode &node, const react::RNSVGRectProps &props) {
  readRenderableProps(node, props);
  node.setGeometry({
      .x = lengthOr(props.x),
      .y = lengthOr(props.y),
      .width = lengthOr(props.width),
      .height = lengthOr(props.height),
      .rx = lengthFromProp(props.rx),
      .ry = lengthFromProp(props.ry),
  });
}

void readLine(LineNode &node, const react::RNSVGLineProps &props) {
  readRenderableProps(node, props);
  node.setGeometry(
      {.x1 = lengthOr(props.x1), .y1 = lengthOr(props.y1), .x2 = lengthOr(props.x2), .y2 = lengthOr(props.y2)});
}

using SvgPathView = SvgNodeViewOf<PathNode, react::RNSVGPathProps, readPath>;
using SvgCircleView = SvgNodeViewOf<CircleNode, react::RNSVGCircleProps, readCircle>;
using SvgEllipseView = SvgNodeViewOf<EllipseNode, react::RNSVGEllipseProps, readEllipse>;
using SvgRectView = SvgNodeViewOf<RectNode, react::RNSVGRectProps, readRect>;
using SvgLineView = SvgNodeViewOf<LineNode, react::RNSVGLineProps, readLine>;

} // namespace

RN_WEBOS_COMPONENT_VIEW("RNSVGPath", SvgPathView);
RN_WEBOS_COMPONENT_VIEW("RNSVGCircle", SvgCircleView);
RN_WEBOS_COMPONENT_VIEW("RNSVGEllipse", SvgEllipseView);
RN_WEBOS_COMPONENT_VIEW("RNSVGRect", SvgRectView);
RN_WEBOS_COMPONENT_VIEW("RNSVGLine", SvgLineView);

} // namespace rnwebos::svg
