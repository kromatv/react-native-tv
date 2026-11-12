#pragma once

#include "SvgFont.h"
#include "SvgNode.h"

#include "include/core/SkTextBlob.h"

#include <optional>
#include <string>
#include <vector>

namespace rnwebos::svg {

/// `<Text>` and `<TSpan>`: one line of glyphs at the text's pen, then the
/// spans inside, which share that pen. `x` and `y` move the pen, `dx` and `dy`
/// shift it, and `textAnchor` aligns what follows up to the next span with an
/// `x` of its own. Each character draws with its own glyph of one font: no
/// ligatures, no fallback to another font, no per-glyph positions or rotation.
class TextNode final : public Node {
 public:
  FontAttributes font;
  std::vector<Length> x;
  std::vector<Length> y;
  std::vector<Length> dx;
  std::vector<Length> dy;

  void setContent(std::string content);

 private:
  void draw(SkCanvas &canvas, const RenderContext &context) const override;
  int contentPaintCount(const RenderContext &context) const override;
  void inherit(RenderContext &context) const override;

  struct Run {
    Font font;
    sk_sp<SkTextBlob> glyphs;
    float advance{0};
  };

  const Run &run(const Font &resolved) const;
  void movePen(SkPoint &pen, SkSize viewport) const;
  /// The advance of the node's glyphs in `own`, its resolved font, and of the
  /// spans after them, up to one with an `x` of its own, which sets `ended`.
  float chunkAdvance(const Font &own, bool &ended) const;
  void drawRun(SkCanvas &canvas, const RenderContext &context, SkPoint &pen) const;

  std::string content_;
  mutable std::optional<Run> run_;
};

} // namespace rnwebos::svg
