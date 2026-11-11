#include "SvgTextNode.h"

#include "../text/TextEngine.h"
#include "SvgBrushPaint.h"

#include "include/core/SkFont.h"
#include "include/core/SkFontStyle.h"
#include "include/core/SkFontTypes.h"
#include "include/core/SkString.h"
#include "include/core/SkTypeface.h"

#include <mutex>
#include <utility>

namespace rnwebos::svg {

namespace {

sk_sp<SkTypeface> typefaceFor(const Font &font) {
  auto &engine = textEngine();
  // FontCollection's face cache is not thread-safe, and Fabric measures on its own thread.
  const std::lock_guard lock(engine.mutex);
  std::vector<SkString> families;
  if (!font.family.empty()) families.emplace_back(font.family.c_str());
  const SkFontStyle style(
      font.weight, SkFontStyle::kNormal_Width, font.italic ? SkFontStyle::kItalic_Slant : SkFontStyle::kUpright_Slant);
  auto found = engine.fonts->findTypefaces(families, style);
  return found.empty() ? nullptr : std::move(found.front());
}

} // namespace

void TextNode::setContent(std::string content) {
  if (content == content_) return;
  content_ = std::move(content);
  run_.reset();
}

const TextNode::Run &TextNode::run(const Font &font) const {
  if (run_ && run_->font == font) return *run_;
  run_ = Run{.font = font};
  auto typeface = typefaceFor(font);
  if (!typeface || content_.empty() || font.size <= 0) return *run_;
  SkFont glyphFont(std::move(typeface), font.size);
  glyphFont.setSubpixel(true);
  glyphFont.setEdging(SkFont::Edging::kAntiAlias);
  run_->glyphs = SkTextBlob::MakeFromText(content_.data(), content_.size(), glyphFont, SkTextEncoding::kUTF8);
  run_->advance = glyphFont.measureText(content_.data(), content_.size(), SkTextEncoding::kUTF8);
  return *run_;
}

void TextNode::movePen(SkPoint &pen, SkSize viewport) const {
  if (!x.empty()) pen.fX = x.front().resolve(viewport.width());
  if (!y.empty()) pen.fY = y.front().resolve(viewport.height());
  if (!dx.empty()) pen.fX += dx.front().resolve(viewport.width());
  if (!dy.empty()) pen.fY += dy.front().resolve(viewport.height());
}

float TextNode::chunkAdvance(const Font &own, bool &ended) const {
  float advance = run(own).advance;
  for (const auto *child : children()) {
    const auto *span = dynamic_cast<const TextNode *>(child);
    if (span == nullptr || span->hidden) continue;
    ended = ended || !span->x.empty();
    if (ended) break;
    advance += span->chunkAdvance(span->font.over(own), ended);
  }
  return advance;
}

void TextNode::drawRun(SkCanvas &canvas, const RenderContext &context, SkPoint &pen) const {
  const auto &shaped = run(context.font);
  if (!shaped.glyphs) return;
  const auto &glyphs = shaped.glyphs;
  const auto bounds = paintsWithReference(context.paint) ? glyphs->bounds().makeOffset(pen) : SkRect::MakeEmpty();
  if (const auto fill = fillPaint(context, bounds)) canvas.drawTextBlob(glyphs, pen.x(), pen.y(), *fill);
  if (const auto stroke = strokePaint(context, bounds)) canvas.drawTextBlob(glyphs, pen.x(), pen.y(), *stroke);
  pen.fX += shaped.advance;
}

void TextNode::draw(SkCanvas &canvas, const RenderContext &context) const {
  TextCursor outermost;
  RenderContext inside = context;
  if (inside.text == nullptr) inside.text = &outermost;
  auto &pen = inside.text->pen;
  movePen(pen, context.viewport);
  if ((context.text == nullptr || !x.empty()) && context.font.anchor != TextAnchor::Start) {
    bool ended = false;
    const float length = chunkAdvance(context.font, ended);
    pen.fX -= context.font.anchor == TextAnchor::Middle ? length / 2 : length;
  }
  drawRun(canvas, inside, pen);
  for (const auto *child : children()) child->render(canvas, inside);
}

int TextNode::contentPaintCount(const RenderContext &context) const {
  using enum Brush::Kind;
  int count = 0;
  const auto &paint = context.paint;
  if (!content_.empty()) count = (paint.fill.kind != None ? 1 : 0) + (paint.stroke.kind != None ? 1 : 0);
  for (const auto *child : children()) {
    if (count > 1) break;
    count += child->paintCount(context);
  }
  return count > 1 ? 2 : count;
}

void TextNode::inherit(RenderContext &context) const {
  context.font = font.over(context.font);
}

} // namespace rnwebos::svg
