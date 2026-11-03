#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace rntizen {

/// A text input's value and selection, edited as an Android EditText edits its
/// own. Positions count Unicode code points, the unit of an Evas textblock
/// cursor and of an input method's offsets; `maxLength` and every offset React
/// Native sees count UTF-16 units, as a JavaScript string does.
class EditBuffer {
 public:
  /// A span of the text, `start <= end`; a caret when they are equal.
  struct Range {
    std::size_t start{0};
    std::size_t end{0};

    bool operator==(const Range &) const = default;
  };

  const std::u32string &text() const;
  std::string utf8() const;
  std::size_t size() const;
  std::size_t utf16Size() const;

  /// Where typing goes: the moving end of the selection.
  std::size_t caret() const;
  Range selection() const;
  bool hasSelection() const;
  /// The selection in UTF-16 units, as `onSelectionChange` reports it.
  Range selectionUtf16() const;

  /// Replaces the whole value and puts the caret at its end.
  void setText(std::string_view utf8);
  /// Selects from `anchor` to `focus`, both UTF-16 offsets, clamped to the text.
  void selectUtf16(std::size_t anchor, std::size_t focus);
  void selectAll();
  /// Puts the caret at code point `position`; `extend` moves only the
  /// selection's moving end.
  void moveTo(std::size_t position, bool extend);

  /// Replaces the selection with `insertion`, cut to the room `maxLength`
  /// UTF-16 units leave (Android's `InputFilter.LengthFilter`). False when
  /// nothing changed.
  bool insert(std::u32string_view insertion, std::size_t maxLength);
  /// Deletes the selection, or the code point before the caret.
  bool deleteBackward();
  /// Deletes the selection, or the code point after the caret.
  bool deleteForward();
  /// Deletes `count` code points starting `offset` code points after the
  /// caret, as an input method's delete-surrounding asks.
  bool deleteAround(long offset, std::size_t count);

  /// The text an input method is still composing. It is part of the value,
  /// as Android's composing span is; any other edit ends it.
  std::optional<Range> composing() const;
  /// Replaces the composition, or the selection, with `text` and keeps it as
  /// the composition, the caret `cursor` code points into it. An empty `text`
  /// removes the composition.
  bool compose(std::u32string_view text, std::size_t cursor, std::size_t maxLength);
  /// Replaces the composition, or the selection, with `text` and ends it.
  bool commit(std::u32string_view text, std::size_t maxLength);
  /// Keeps the composed text as typed text.
  void finishComposing();

 private:
  bool replace(Range range, std::u32string_view insertion, std::size_t maxLength);
  std::size_t utf16Offset(std::size_t position) const;
  std::size_t positionAtUtf16(std::size_t offset) const;
  bool erase(std::size_t start, std::size_t end);

  std::u32string text_;
  std::size_t anchor_{0};
  std::size_t focus_{0};
  std::optional<Range> composing_;
};

std::u32string decodeUtf8(std::string_view utf8);
std::string encodeUtf8(std::u32string_view text);
std::size_t utf16Length(std::u32string_view text);

} // namespace rntizen
