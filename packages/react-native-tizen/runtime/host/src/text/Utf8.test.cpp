#include "Utf8.h"

#include <doctest/doctest.h>

#include <string>

namespace rntizen {

TEST_CASE("decodeUtf8 decodes every sequence length") {
  CHECK(decodeUtf8("a\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80") == U"aé€\U0001F600");
  CHECK(decodeUtf8("\xF4\x8F\xBF\xBF") == U"\U0010FFFF");
}

TEST_CASE("decodeUtf8 never swallows the character after a truncated sequence") {
  CHECK(decodeUtf8("\xE2\x82" "a") == U"�a");
  CHECK(decodeUtf8("\xC3" "b") == U"�b");
  CHECK(decodeUtf8("\xF0\x9F\x98") == U"�");
}

TEST_CASE("decodeUtf8 replaces each maximal invalid subpart, as Unicode's example does") {
  const std::string bytes = "\x61\xF1\x80\x80\xE1\x80\xC2\x62\x80\x63\x80\xBF\x64";

  CHECK(decodeUtf8(bytes) == U"a���b�c��d");
}

TEST_CASE("decodeUtf8 rejects overlong forms, surrogates and code points past U+10FFFF") {
  CHECK(decodeUtf8("\xC0\xAF") == U"��");
  CHECK(decodeUtf8("\xE0\x80\xAF") == U"���");
  CHECK(decodeUtf8("\xED\xA0\x80") == U"���");
  CHECK(decodeUtf8("\xF0\x80\x80\x80") == U"����");
  CHECK(decodeUtf8("\xF4\x90\x80\x80") == U"����");
  CHECK(decodeUtf8("\xF5\xFF") == U"��");
}

TEST_CASE("encodeUtf8 reverses decodeUtf8") {
  const std::string text = "a\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80";

  CHECK(encodeUtf8(decodeUtf8(text)) == text);
}

} // namespace rntizen
