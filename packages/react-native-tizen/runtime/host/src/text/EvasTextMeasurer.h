#pragma once

#include "UnwrappedSizes.h"

#include <react/renderer/textlayoutmanager/TextMeasurer.h>

#include <Ecore_Evas.h>

namespace rntizen {

/// Measures paragraphs with an Evas textblock on an offscreen canvas, the same
/// engine ParagraphView draws with. Evas is single-threaded and its font caches
/// are shared by every canvas (efl-rendering.md section 7), so a measurement
/// from Fabric's layout thread runs on the main loop, ahead of queued work and
/// between the mutations of a mount (`runOnMainThreadSoon`). A paragraph that
/// fits unwrapped is measured once for every width it fits. Create and destroy
/// it on the main thread.
class EvasTextMeasurer final : public facebook::react::TextMeasurer {
 public:
  EvasTextMeasurer();
  ~EvasTextMeasurer() override;

  EvasTextMeasurer(const EvasTextMeasurer &) = delete;
  EvasTextMeasurer &operator=(const EvasTextMeasurer &) = delete;

  facebook::react::Size measure(
      const facebook::react::AttributedString &attributedString,
      const facebook::react::ParagraphAttributes &paragraphAttributes,
      facebook::react::Size maximumSize,
      facebook::react::Float pointScaleFactor) const override;

 private:
  Ecore_Evas *canvas_;
  Evas_Object *textblock_;
  Evas_Textblock_Style *style_;
  UnwrappedSizes unwrapped_;
};

} // namespace rntizen
