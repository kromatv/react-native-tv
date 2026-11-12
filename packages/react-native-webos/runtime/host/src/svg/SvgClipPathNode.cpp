#include "SvgClipPathNode.h"

#include "include/core/SkPathBuilder.h"
#include "include/core/SkPathTypes.h"
#include "include/pathops/SkPathOps.h"

namespace rnwebos::svg {

namespace {

constexpr int kEvenOdd = 0;

SkPath together(const std::vector<SkPath> &outlines, SkPathFillType fillType) {
  SkPathBuilder builder(fillType);
  for (const auto &outline : outlines) builder.addPath(outline);
  return builder.detach();
}

} // namespace

bool ClipPathNode::definesOnly() const {
  return true;
}

SkPath ClipPathNode::outline(int clipRule, const RenderContext &context) const {
  std::vector<SkPath> outlines;
  appendContentOutlines(outlines, matrix, context);
  if (clipRule == kEvenOdd) return together(outlines, SkPathFillType::kEvenOdd);
  if (outlines.size() == 1) return outlines.front().makeFillType(SkPathFillType::kWinding);
  SkOpBuilder joined;
  for (const auto &outline : outlines) joined.add(outline.makeFillType(SkPathFillType::kWinding), kUnion_SkPathOp);
  if (auto result = joined.resolve()) return *result;
  return together(outlines, SkPathFillType::kWinding);
}

} // namespace rnwebos::svg
