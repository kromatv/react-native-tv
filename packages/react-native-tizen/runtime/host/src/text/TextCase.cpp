#include "TextCase.h"

#include "Utf8.h"

namespace rntizen {

namespace {

bool inRange(char32_t c, char32_t first, char32_t last) {
  return c >= first && c <= last;
}

bool evenIsUpper(char32_t c) {
  return inRange(c, 0x100, 0x137) || inRange(c, 0x14A, 0x177);
}

bool oddIsUpper(char32_t c) {
  return inRange(c, 0x139, 0x148) || inRange(c, 0x179, 0x17E);
}

char32_t upper(char32_t c) {
  if (inRange(c, 'a', 'z') || (inRange(c, 0xE0, 0xFE) && c != 0xF7)) return c - 0x20;
  if (c == 0xFF) return 0x178;
  if (c == 0x131) return 'I';
  if (evenIsUpper(c) && c % 2 == 1) return c - 1;
  if (oddIsUpper(c) && c % 2 == 0) return c - 1;
  return c;
}

char32_t lower(char32_t c) {
  if (inRange(c, 'A', 'Z') || (inRange(c, 0xC0, 0xDE) && c != 0xD7)) return c + 0x20;
  if (c == 0x178) return 0xFF;
  if (c == 0x130) return 'i';
  if (evenIsUpper(c) && c % 2 == 0) return c + 1;
  if (oddIsUpper(c) && c % 2 == 1) return c + 1;
  return c;
}

bool isSpace(char32_t c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == 0xA0;
}

} // namespace

std::string transformedText(const std::string &text, facebook::react::TextTransform textTransform) {
  using enum facebook::react::TextTransform;
  if (textTransform != Uppercase && textTransform != Lowercase && textTransform != Capitalize) return text;
  auto codePoints = decodeUtf8(text);
  bool wordStart = true;
  for (auto &c : codePoints) {
    if (textTransform == Uppercase) {
      c = upper(c);
    } else if (textTransform == Lowercase) {
      c = lower(c);
    } else if (wordStart) {
      c = upper(c);
    }
    wordStart = isSpace(c);
  }
  return encodeUtf8(codePoints);
}

} // namespace rntizen
