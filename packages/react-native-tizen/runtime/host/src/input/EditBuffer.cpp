#include "EditBuffer.h"

#include <algorithm>

namespace rntizen {

namespace {

constexpr char32_t kReplacement = 0xFFFD;

std::size_t utf16Units(char32_t codePoint) {
  return codePoint > 0xFFFF ? 2 : 1;
}

} // namespace

std::u32string decodeUtf8(std::string_view utf8) {
  std::u32string decoded;
  decoded.reserve(utf8.size());
  for (std::size_t i = 0; i < utf8.size();) {
    const auto lead = static_cast<unsigned char>(utf8[i]);
    const std::size_t length = lead < 0x80 ? 1 : (lead >> 5) == 0x6 ? 2 : (lead >> 4) == 0xE ? 3 : (lead >> 3) == 0x1E ? 4 : 0;
    if (length == 0 || i + length > utf8.size()) {
      decoded.push_back(kReplacement);
      ++i;
      continue;
    }
    char32_t codePoint = length == 1 ? lead : lead & (0x7F >> length);
    for (std::size_t k = 1; k < length; ++k) {
      codePoint = (codePoint << 6) | (static_cast<unsigned char>(utf8[i + k]) & 0x3F);
    }
    decoded.push_back(codePoint);
    i += length;
  }
  return decoded;
}

std::string encodeUtf8(std::u32string_view text) {
  std::string encoded;
  encoded.reserve(text.size());
  for (const char32_t codePoint : text) {
    if (codePoint < 0x80) {
      encoded.push_back(static_cast<char>(codePoint));
    } else if (codePoint < 0x800) {
      encoded.push_back(static_cast<char>(0xC0 | (codePoint >> 6)));
      encoded.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
    } else if (codePoint < 0x10000) {
      encoded.push_back(static_cast<char>(0xE0 | (codePoint >> 12)));
      encoded.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F)));
      encoded.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
    } else {
      encoded.push_back(static_cast<char>(0xF0 | (codePoint >> 18)));
      encoded.push_back(static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F)));
      encoded.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F)));
      encoded.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
    }
  }
  return encoded;
}

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
  anchor_ = focus_ = text_.size();
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
  anchor_ = focus_ = range.start + std::min(cursor, accepted);
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
  anchor_ = focus_ = range.start + accepted;
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
  anchor_ = focus_ = start;
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
