#pragma once

#include "views/PixelGeometry.h"

#include <Ecore.h>
#include <Evas.h>

#include <string>

namespace rntizen::expo {

/// The subtitle line a video view draws over its box: white text with a black
/// outline, centered near the bottom, for as long as the player says. Main thread only.
class SubtitleOverlay final {
 public:
  explicit SubtitleOverlay(Evas *evas);
  ~SubtitleOverlay();

  SubtitleOverlay(const SubtitleOverlay &) = delete;
  SubtitleOverlay &operator=(const SubtitleOverlay &) = delete;

  Evas_Object *object() const;

  /// Shows `text` for `milliseconds`; empty text clears the line.
  void show(const std::string &text, unsigned long milliseconds);
  void clear();
  /// Lays the line out in `box`, with text sized for `scale` device pixels per point.
  void place(const PixelRect &box, double scale);

 private:
  void layout();
  static Eina_Bool expire(void *data);

  Evas_Textblock_Style *const style_;
  Evas_Object *const text_;
  Ecore_Timer *timer_{nullptr};
  PixelRect box_;
  int fontSize_{0};
  bool shown_{false};
};

} // namespace rntizen::expo
