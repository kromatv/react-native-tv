#include "SvgPatternNode.h"

#include "include/core/SkPicture.h"
#include "include/core/SkPictureRecorder.h"
#include "include/core/SkSamplingOptions.h"
#include "include/core/SkTileMode.h"

namespace rnwebos::svg {

bool PatternNode::definesOnly() const {
  return true;
}

SkRect PatternNode::tile(const SkRect &bounds, SkSize viewport) const {
  if (tileInUserSpace) {
    return SkRect::MakeXYWH(
        x.resolve(viewport.width()),
        y.resolve(viewport.height()),
        width.resolve(viewport.width()),
        height.resolve(viewport.height()));
  }
  return SkRect::MakeXYWH(
      bounds.left() + x.fractionOf(bounds.width()),
      bounds.top() + y.fractionOf(bounds.height()),
      width.fractionOf(bounds.width()),
      height.fractionOf(bounds.height()));
}

sk_sp<SkShader> PatternNode::shader(const SkRect &bounds, const RenderContext &context) const {
  const auto area = tile(bounds, context.viewport);
  if (area.isEmpty()) return nullptr;
  const auto extent = SkRect::MakeWH(area.width(), area.height());
  SkPictureRecorder recorder;
  auto *canvas = recorder.beginRecording(extent);
  if (viewBox.present()) {
    canvas->concat(viewBoxTransform(viewBox, area.width(), area.height()));
  } else if (contentInBoundingBox) {
    canvas->scale(bounds.width(), bounds.height());
  }
  RenderContext inside{
      .paint = paint,
      .currentColor = color.value_or(context.currentColor),
      .viewport = context.viewport,
      .definitions = context.definitions,
      .depth = context.depth + 1,
  };
  inherit(inside);
  GroupNode::draw(*canvas, inside);
  auto placement = transform;
  placement.preTranslate(area.left(), area.top());
  return recorder.finishRecordingAsPicture()->makeShader(
      SkTileMode::kRepeat, SkTileMode::kRepeat, SkFilterMode::kLinear, &placement, &extent);
}

} // namespace rnwebos::svg
