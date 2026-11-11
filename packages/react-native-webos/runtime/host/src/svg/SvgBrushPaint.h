#pragma once

#include "SvgNode.h"

#include "include/core/SkPaint.h"
#include "include/core/SkPathTypes.h"
#include "include/core/SkRect.h"

#include <optional>

namespace rnwebos::svg {

/// Whether a gradient fills or strokes with `paint`, which takes the bounds of
/// what it paints.
bool paintsWithReference(const Paint &paint);

/// The paint that fills a geometry spanning `bounds` in `context`, or none
/// when nothing would show.
std::optional<SkPaint> fillPaint(const RenderContext &context, const SkRect &bounds);

/// The paint that strokes a geometry spanning `bounds` in `context`: width,
/// cap, join, miter limit and dashes, or none when nothing would show.
std::optional<SkPaint> strokePaint(const RenderContext &context, const SkRect &bounds);

SkPathFillType fillTypeOf(int fillRule);

} // namespace rnwebos::svg
