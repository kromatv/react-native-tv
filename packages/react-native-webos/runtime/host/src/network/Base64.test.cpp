#include "Base64.h"

#include <doctest/doctest.h>

#include <stdexcept>
#include <string>

namespace rntizen {

TEST_CASE("encodeBase64 pads the RFC 4648 test vectors") {
  CHECK(encodeBase64("") == "");
  CHECK(encodeBase64("f") == "Zg==");
  CHECK(encodeBase64("fo") == "Zm8=");
  CHECK(encodeBase64("foo") == "Zm9v");
  CHECK(encodeBase64("foob") == "Zm9vYg==");
  CHECK(encodeBase64("fooba") == "Zm9vYmE=");
  CHECK(encodeBase64("foobar") == "Zm9vYmFy");
}

TEST_CASE("encodeBase64 reads every byte as unsigned") {
  const std::string bytes{'\xff', '\xfe', '\x00', '\x80'};

  CHECK(encodeBase64(bytes) == "//4AgA==");
}

TEST_CASE("decodeBase64 reverses encodeBase64") {
  const std::string bytes{'\x00', '\x01', '\x7f', '\x80', '\xff', 'k', 'r'};

  CHECK(decodeBase64(encodeBase64(bytes)) == bytes);
}

TEST_CASE("decodeBase64 skips whitespace and stops at padding") {
  CHECK(decodeBase64("Zm9v\r\nYmFy") == "foobar");
  CHECK(decodeBase64(" Zm8=\t") == "fo");
  CHECK(decodeBase64("Zg==Zm9v") == "f");
  CHECK(decodeBase64("Zm\t9v") == "foo");
}

TEST_CASE("decodeBase64 rejects a character outside the alphabet") {
  CHECK_THROWS_AS(decodeBase64("Zm9v-"), std::invalid_argument);
  CHECK_THROWS_AS(decodeBase64("Zm\x80v"), std::invalid_argument);
}

} // namespace rntizen
