#pragma once

#include "../text/TextblockMarkup.h"
#include "PixelGeometry.h"

#include <react/renderer/attributedstring/primitives.h>
#include <react/renderer/graphics/Color.h>

#include <Ecore.h>
#include <Evas.h>

#include <cstddef>
#include <optional>

namespace rntizen {

/// What a text input draws inside its padding: its text, or its placeholder,
/// in one Evas textblock built like a `<Text>`'s, the selection and the
/// composition as that text's own markup, and a caret blinking every 500 ms as
/// Android's does. Scrolled so the caret stays in the box: sideways on one
/// line, down on several. Main thread only.
class TextInputLayer final {
 public:
  explicit TextInputLayer(Evas *evas);
  ~TextInputLayer();

  TextInputLayer(const TextInputLayer &) = delete;
  TextInputLayer &operator=(const TextInputLayer &) = delete;

  /// The clip of the content box: a member below the children.
  Evas_Object *clip() const;
  /// The textblock and the caret: members drawn above, clipped by clip().
  Evas_Object *text() const;
  Evas_Object *caret() const;

  /// Shows `content`, with the caret before character `caret` of it.
  void setContent(const TextblockContent &content, std::size_t caret, bool multiline);
  void setVerticalAlignment(std::optional<facebook::react::TextAlignmentVertical> alignment);
  /// Shows the caret, blinking, in `color`; hides it when `shown` is false.
  void setCaret(bool shown, const facebook::react::SharedColor &color);
  /// Places everything inside `box`, the content box in canvas pixels, with a
  /// caret `caretWidth` pixels wide.
  void layout(const PixelRect &box, Evas_Coord caretWidth);

  /// The caret on the canvas, in pixels.
  PixelRect caretRect() const;
  /// The laid-out text's size, in pixels.
  Evas_Coord textWidth() const;
  Evas_Coord textHeight() const;
  /// Where the caret lands one line up (`lines` -1) or down (1), as a
  /// character index; none past the first or the last line.
  std::optional<std::size_t> caretOnLine(int lines) const;

 private:
  static Eina_Bool onBlink(void *self);
  void restartBlink();
  void placeCaret();

  Evas_Object *const clip_;
  Evas_Object *const text_;
  Evas_Object *const caret_;
  Evas_Textblock_Style *const style_;
  Evas_Textblock_Cursor *const cursor_;
  Ecore_Timer *blink_{nullptr};
  std::optional<facebook::react::TextAlignmentVertical> verticalAlignment_;
  PixelRect box_{};
  PixelRect caretInText_{};
  Evas_Coord textWidth_{0};
  Evas_Coord textHeight_{0};
  Evas_Coord caretWidth_{1};
  Evas_Coord scrollX_{0};
  Evas_Coord scrollY_{0};
  std::size_t caretIndex_{0};
  bool multiline_{false};
  bool caretShown_{false};
  bool caretLit_{true};
};

} // namespace rntizen
