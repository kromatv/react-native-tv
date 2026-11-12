#include "OverlayText.h"

#include "../text/TextEngine.h"

#include "include/core/SkFontStyle.h"
#include "modules/skparagraph/include/ParagraphBuilder.h"
#include "modules/skparagraph/include/ParagraphStyle.h"
#include "modules/skparagraph/include/TextStyle.h"

#include <algorithm>

namespace rnwebos {

std::unique_ptr<skia::textlayout::Paragraph> overlayParagraph(const std::vector<OverlayRun> &runs, float width) {
  using namespace skia::textlayout;
  auto &engine = textEngine();
  std::lock_guard lock(engine.mutex);
  auto builder = ParagraphBuilder::make(ParagraphStyle(), engine.fonts, engine.unicode);
  for (const auto &run : runs) {
    TextStyle style;
    style.setColor(run.color);
    style.setFontSize(run.size);
    if (run.bold) style.setFontStyle(SkFontStyle::Bold());
    if (run.monospace) style.setFontFamilies({SkString("monospace"), SkString("DejaVu Sans Mono")});
    builder->pushStyle(style);
    builder->addText(run.text.data(), run.text.size());
    builder->pop();
  }
  auto paragraph = builder->Build();
  paragraph->layout(std::max(1.0f, width));
  return paragraph;
}

} // namespace rnwebos
