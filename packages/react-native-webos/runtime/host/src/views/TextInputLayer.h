#pragma once

#include "../platform/MainLoop.h"
#include "../render/Scene.h"
#include "../text/ParagraphLayout.h"
#include "PixelGeometry.h"

#include <react/renderer/attributedstring/primitives.h>

#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkRect.h"

#include <cstddef>
#include <optional>
#include <vector>

namespace rnwebos {

/// What a text input draws inside its padding: its text, or its placeholder,
/// laid out by SkParagraph like a `<Text>`'s, the selection behind it, and a
/// caret blinking every 500 ms as Android's does. Scrolled so the caret stays
/// in the box: sideways on one line, down on several. Offsets count UTF-16
/// units of the text shown, as SkParagraph's do. Main thread only.
class TextInputLayer final {
 public:
  /// The scene is redrawn whenever what the layer shows changes.
  explicit TextInputLayer(Scene &scene);
  ~TextInputLayer();

  TextInputLayer(const TextInputLayer &) = delete;
  TextInputLayer &operator=(const TextInputLayer &) = delete;

  /// Shows `text`, on one unbroken line unless `multiline`.
  void setText(
      const facebook::react::AttributedString &text,
      const facebook::react::ParagraphAttributes &attributes,
      bool multiline);
  /// Puts the caret before unit `caret` and highlights `[start, end)` in `color`.
  void setSelection(std::size_t caret, std::size_t start, std::size_t end, SkColor4f color);
  void setVerticalAlignment(std::optional<facebook::react::TextAlignmentVertical> alignment);
  /// Shows the caret, blinking, in `color`; hides it when `shown` is false.
  void setCaret(bool shown, SkColor4f color);
  /// Places everything inside `box`, the content box in the view's own
  /// pixels, at `scale` pixels per point, with a caret `caretWidth` pixels wide.
  void layout(const PixelRect &box, facebook::react::Float scale, int caretWidth);

  /// Draws on the view's canvas, clipped to the content box.
  void draw(SkCanvas &canvas);

  /// The caret in the view's own pixels.
  PixelRect caretRect();
  /// The laid-out text's size, in pixels.
  PixelSize textSize();
  /// Where the caret lands one line up (`lines` -1) or down (1); none past
  /// the first or the last line, or on a single line.
  std::optional<std::size_t> caretOnLine(int lines);

 private:
  struct Line {
    std::size_t start;
    std::size_t end;
    float left;
    float top;
    float height;
  };

  bool ensureLaidOut();
  void readLines();
  void place();
  std::size_t lineOf(std::size_t offset) const;
  float caretX(const Line &line, std::size_t offset) const;
  void restartBlink();
  void armBlink();

  Scene &scene_;
  facebook::react::AttributedString text_;
  facebook::react::ParagraphAttributes attributes_;
  std::optional<facebook::react::TextAlignmentVertical> verticalAlignment_;
  ParagraphLayout layout_;
  std::vector<Line> lines_;
  std::vector<SkRect> selectionRects_;
  SkRect caretInText_{};
  SkColor4f caretColor_{SkColors::kBlack};
  SkColor4f selectionColor_{SkColors::kTransparent};
  PixelRect box_{};
  facebook::react::Float scale_{1};
  MainLoop::Id blink_{0};
  std::size_t caret_{0};
  std::size_t selectionStart_{0};
  std::size_t selectionEnd_{0};
  int textWidth_{0};
  int textHeight_{0};
  int caretWidth_{1};
  int scrollX_{0};
  int scrollY_{0};
  int offsetY_{0};
  bool multiline_{false};
  bool placed_{false};
  bool caretShown_{false};
  bool caretLit_{true};
};

} // namespace rnwebos
