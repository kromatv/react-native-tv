#pragma once

#include "include/core/SkColor.h"
#include "modules/skparagraph/include/Paragraph.h"

#include <memory>
#include <string>
#include <vector>

namespace rnwebos {

/// One run of the text the host itself draws: the error screen, an alert, the
/// loading banner.
struct OverlayRun {
  std::string text;
  float size{28};
  SkColor color{SK_ColorWHITE};
  bool bold{false};
  bool monospace{false};
};

/// `runs` as one paragraph wrapped at `width` pixels, ready to paint. Main
/// thread or any thread: it takes the text engine's lock.
std::unique_ptr<skia::textlayout::Paragraph> overlayParagraph(const std::vector<OverlayRun> &runs, float width);

} // namespace rnwebos
