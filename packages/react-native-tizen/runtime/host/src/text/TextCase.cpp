#include "TextCase.h"

#include <vector>

namespace rntizen {

namespace {

using facebook::react::TextTransform;

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

std::vector<char32_t> decode(const std::string &text) {
  std::vector<char32_t> codePoints;
  for (std::size_t i = 0; i < text.size();) {
    const auto lead = static_cast<unsigned char>(text[i]);
    const int length = lead < 0x80 ? 1 : lead < 0xE0 ? 2 : lead < 0xF0 ? 3 : 4;
    char32_t c = length == 1 ? lead : lead & (0xFF >> (length + 1));
    for (int k = 1; k < length && i + static_cast<std::size_t>(k) < text.size(); ++k) {
      c = (c << 6) | (static_cast<unsigned char>(text[i + static_cast<std::size_t>(k)]) & 0x3F);
    }
    codePoints.push_back(c);
    i += static_cast<std::size_t>(length);
  }
  return codePoints;
}

void encode(char32_t c, std::string &out) {
  if (c < 0x80) {
    out += static_cast<char>(c);
  } else if (c < 0x800) {
    out += static_cast<char>(0xC0 | (c >> 6));
    out += static_cast<char>(0x80 | (c & 0x3F));
  } else if (c < 0x10000) {
    out += static_cast<char>(0xE0 | (c >> 12));
    out += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
    out += static_cast<char>(0x80 | (c & 0x3F));
  } else {
    out += static_cast<char>(0xF0 | (c >> 18));
    out += static_cast<char>(0x80 | ((c >> 12) & 0x3F));
    out += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
    out += static_cast<char>(0x80 | (c & 0x3F));
  }
}

bool isSpace(char32_t c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == 0xA0;
}

} // namespace

std::string transformedText(const std::string &text, TextTransform textTransform) {
  if (textTransform != TextTransform::Uppercase && textTransform != TextTransform::Lowercase &&
      textTransform != TextTransform::Capitalize) {
    return text;
  }
  std::string out;
  out.reserve(text.size());
  bool wordStart = true;
  for (auto c : decode(text)) {
    if (textTransform == TextTransform::Uppercase) {
      c = upper(c);
    } else if (textTransform == TextTransform::Lowercase) {
      c = lower(c);
    } else if (wordStart) {
      c = upper(c);
    }
    wordStart = isSpace(c);
    encode(c, out);
  }
  return out;
}

} // namespace rntizen
