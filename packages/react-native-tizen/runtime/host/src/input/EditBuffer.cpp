#include "EditBuffer.h"

#include <algorithm>

namespace rntizen {

namespace {

std::size_t utf16Units(char32_t codePoint) {
  return codePoint > 0xFFFF ? 2 : 1;
}

} // namespace

std::size_t utf16Length(std::u32string_view text) {
  std::size_t length = 0;
  for (const char32_t codePoint : text) length += utf16Units(codePoint);
  return length;
}

const std::u32string &EditBuffer::text() const {
  return text_;
}

std::string EditBuffer::utf8() const {
  return encodeUtf8(text_);
}

std::size_t EditBuffer::size() const {
  return text_.size();
}

std::size_t EditBuffer::utf16Size() const {
  return utf16Length(text_);
}

std::size_t EditBuffer::caret() const {
  return focus_;
}

EditBuffer::Range EditBuffer::selection() const {
  return {.start = std::min(anchor_, focus_), .end = std::max(anchor_, focus_)};
}

bool EditBuffer::hasSelection() const {
  return anchor_ != focus_;
}

EditBuffer::Range EditBuffer::selectionUtf16() const {
  const auto range = selection();
  return {.start = utf16Offset(range.start), .end = utf16Offset(range.end)};
}

void EditBuffer::setText(std::string_view utf8) {
  text_ = decodeUtf8(utf8);
  focus_ = text_.size();
  anchor_ = focus_;
  composing_.reset();
}

void EditBuffer::selectUtf16(std::size_t anchor, std::size_t focus) {
  anchor_ = positionAtUtf16(anchor);
  focus_ = positionAtUtf16(focus);
  composing_.reset();
}

void EditBuffer::selectAll() {
  anchor_ = 0;
  focus_ = text_.size();
  composing_.reset();
}

void EditBuffer::moveTo(std::size_t position, bool extend) {
  focus_ = std::min(position, text_.size());
  if (!extend) anchor_ = focus_;
  composing_.reset();
}

bool EditBuffer::insert(std::u32string_view insertion, std::size_t maxLength) {
  composing_.reset();
  return replace(selection(), insertion, maxLength);
}

std::optional<EditBuffer::Range> EditBuffer::composing() const {
  return composing_;
}

bool EditBuffer::compose(std::u32string_view text, std::size_t cursor, std::size_t maxLength) {
  const auto range = composing_.value_or(selection());
  const bool changed = replace(range, text, maxLength);
  const auto accepted = focus_ - range.start;
  composing_.reset();
  if (accepted > 0) composing_ = Range{.start = range.start, .end = focus_};
  focus_ = range.start + std::min(cursor, accepted);
  anchor_ = focus_;
  return changed;
}

bool EditBuffer::commit(std::u32string_view text, std::size_t maxLength) {
  const auto range = composing_.value_or(selection());
  composing_.reset();
  return replace(range, text, maxLength);
}

void EditBuffer::finishComposing() {
  composing_.reset();
}

bool EditBuffer::replace(Range range, std::u32string_view insertion, std::size_t maxLength) {
  const auto replaced = utf16Length(std::u32string_view(text_).substr(range.start, range.end - range.start));
  const auto kept = utf16Size() - replaced;
  const auto room = maxLength > kept ? maxLength - kept : 0;
  std::size_t accepted = 0;
  std::size_t units = 0;
  while (accepted < insertion.size() && units + utf16Units(insertion[accepted]) <= room) {
    units += utf16Units(insertion[accepted]);
    ++accepted;
  }
  focus_ = range.start + accepted;
  anchor_ = focus_;
  if (accepted == 0 && range.start == range.end) return false;
  const auto keptInsertion = insertion.substr(0, accepted);
  if (std::u32string_view(text_).substr(range.start, range.end - range.start) == keptInsertion) return false;
  text_.replace(range.start, range.end - range.start, keptInsertion);
  return true;
}

bool EditBuffer::deleteBackward() {
  const auto range = selection();
  if (range.start != range.end) return erase(range.start, range.end);
  return range.start > 0 && erase(range.start - 1, range.start);
}

bool EditBuffer::deleteForward() {
  const auto range = selection();
  if (range.start != range.end) return erase(range.start, range.end);
  return range.end < text_.size() && erase(range.end, range.end + 1);
}

bool EditBuffer::deleteAround(long offset, std::size_t count) {
  const auto from = std::clamp<long>(static_cast<long>(focus_) + offset, 0, static_cast<long>(text_.size()));
  const auto start = static_cast<std::size_t>(from);
  return erase(start, std::min(text_.size(), start + count));
}

bool EditBuffer::erase(std::size_t start, std::size_t end) {
  composing_.reset();
  if (start >= end) return false;
  text_.erase(start, end - start);
  focus_ = start;
  anchor_ = focus_;
  return true;
}

std::size_t EditBuffer::utf16Offset(std::size_t position) const {
  return utf16Length(std::u32string_view(text_).substr(0, std::min(position, text_.size())));
}

std::size_t EditBuffer::positionAtUtf16(std::size_t offset) const {
  std::size_t units = 0;
  std::size_t position = 0;
  while (position < text_.size() && units + utf16Units(text_[position]) <= offset) {
    units += utf16Units(text_[position]);
    ++position;
  }
  return position;
}

} // namespace rntizen
