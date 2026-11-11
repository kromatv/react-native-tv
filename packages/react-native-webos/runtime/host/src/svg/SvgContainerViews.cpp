#include "../mounting/ComponentViewRegistry.h"
#include "SvgClipPathNode.h"
#include "SvgGroupNode.h"
#include "SvgMaskNode.h"
#include "SvgNodeProps.h"
#include "SvgNodeView.h"
#include "SvgPatternNode.h"
#include "SvgUseNode.h"

#include <react/renderer/components/rnsvg/Props.h>

namespace rnwebos::svg {

namespace {

namespace react = facebook::react;

constexpr int kUserSpaceOnUse = 1;
constexpr int kObjectBoundingBox = 0;
constexpr int kAlphaMask = 1;
constexpr Length kNone{.value = 0, .percent = true};
constexpr Length kWhole{.value = 100, .percent = true};

template <typename Props>
void readGroupProps(GroupNode &node, const Props &props) {
  readRenderableProps(node, props);
  node.font = fontAttributesFromProp(props.font);
}

void readGroup(GroupNode &node, const react::RNSVGGroupProps &props) {
  readGroupProps(node, props);
}

void readDefs(DefinitionsNode &node, const react::RNSVGDefsProps &props) {
  readNodeProps(node, props);
}

void readClipPath(ClipPathNode &node, const react::RNSVGClipPathProps &props) {
  readGroupProps(node, props);
}

void readMask(MaskNode &node, const react::RNSVGMaskProps &props) {
  readGroupProps(node, props);
  node.x = lengthOr(props.x, kNone);
  node.y = lengthOr(props.y, kNone);
  node.width = lengthOr(props.width, kWhole);
  node.height = lengthOr(props.height, kWhole);
  node.areaInUserSpace = props.maskUnits == kUserSpaceOnUse;
  node.contentInBoundingBox = props.maskContentUnits == kObjectBoundingBox;
  node.alphaOnly = props.maskType == kAlphaMask;
}

template <typename Props>
SvgViewBox viewBoxOf(const Props &props) {
  return {
      .minX = props.minX,
      .minY = props.minY,
      .width = props.vbWidth,
      .height = props.vbHeight,
      .align = props.align,
      .meetOrSlice = props.meetOrSlice,
  };
}

void readSymbol(SymbolNode &node, const react::RNSVGSymbolProps &props) {
  readGroupProps(node, props);
  node.viewBox = viewBoxOf(props);
}

void readPattern(PatternNode &node, const react::RNSVGPatternProps &props) {
  readGroupProps(node, props);
  node.x = lengthOr(props.x, kNone);
  node.y = lengthOr(props.y, kNone);
  node.width = lengthOr(props.width, kWhole);
  node.height = lengthOr(props.height, kWhole);
  node.tileInUserSpace = props.patternUnits == kUserSpaceOnUse;
  node.contentInBoundingBox = props.patternContentUnits == kObjectBoundingBox;
  node.transform = matrixFromProp(props.patternTransform);
  node.viewBox = viewBoxOf(props);
}

void readUse(UseNode &node, const react::RNSVGUseProps &props) {
  readRenderableProps(node, props);
  node.href = props.href;
  node.x = lengthOr(props.x);
  node.y = lengthOr(props.y);
  node.width = lengthOr(props.width);
  node.height = lengthOr(props.height);
}

using SvgGroupView = SvgNodeViewOf<GroupNode, react::RNSVGGroupProps, readGroup>;
using SvgDefsView = SvgNodeViewOf<DefinitionsNode, react::RNSVGDefsProps, readDefs>;
using SvgClipPathView = SvgNodeViewOf<ClipPathNode, react::RNSVGClipPathProps, readClipPath>;
using SvgMaskView = SvgNodeViewOf<MaskNode, react::RNSVGMaskProps, readMask>;
using SvgSymbolView = SvgNodeViewOf<SymbolNode, react::RNSVGSymbolProps, readSymbol>;
using SvgUseView = SvgNodeViewOf<UseNode, react::RNSVGUseProps, readUse>;
using SvgPatternView = SvgNodeViewOf<PatternNode, react::RNSVGPatternProps, readPattern>;

} // namespace

RN_WEBOS_COMPONENT_VIEW("RNSVGGroup", SvgGroupView);
RN_WEBOS_COMPONENT_VIEW("RNSVGDefs", SvgDefsView);
RN_WEBOS_COMPONENT_VIEW("RNSVGClipPath", SvgClipPathView);
RN_WEBOS_COMPONENT_VIEW("RNSVGMask", SvgMaskView);
RN_WEBOS_COMPONENT_VIEW("RNSVGSymbol", SvgSymbolView);
RN_WEBOS_COMPONENT_VIEW("RNSVGUse", SvgUseView);
RN_WEBOS_COMPONENT_VIEW("RNSVGPattern", SvgPatternView);

} // namespace rnwebos::svg
