#include "../mounting/ComponentViewRegistry.h"
#include "SvgNodeProps.h"
#include "SvgNodeView.h"
#include "SvgTextNode.h"

#include <react/renderer/components/rnsvg/Props.h>

namespace rnwebos::svg {

namespace {

namespace react = facebook::react;

template <typename Props>
void readTextProps(TextNode &node, const Props &props) {
  readRenderableProps(node, props);
  node.font = fontAttributesFromProp(props.font);
  node.x = lengthsFromProp(props.x);
  node.y = lengthsFromProp(props.y);
  node.dx = lengthsFromProp(props.dx);
  node.dy = lengthsFromProp(props.dy);
}

void readText(TextNode &node, const react::RNSVGTextProps &props) {
  readTextProps(node, props);
}

void readSpan(TextNode &node, const react::RNSVGTSpanProps &props) {
  readTextProps(node, props);
  node.setContent(props.content);
}

using SvgTextView = SvgNodeViewOf<TextNode, react::RNSVGTextProps, readText>;
using SvgSpanView = SvgNodeViewOf<TextNode, react::RNSVGTSpanProps, readSpan>;

} // namespace

RN_WEBOS_COMPONENT_VIEW("RNSVGText", SvgTextView);
RN_WEBOS_COMPONENT_VIEW("RNSVGTSpan", SvgSpanView);

} // namespace rnwebos::svg
