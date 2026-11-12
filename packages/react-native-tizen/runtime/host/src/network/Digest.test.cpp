#include "Digest.h"

#include <doctest/doctest.h>

#include <array>
#include <cstdint>
#include <string>

namespace rntizen {

namespace {

std::string hex(const std::array<std::uint8_t, 20> &digest) {
  constexpr const char *kDigits = "0123456789abcdef";
  std::string text;
  for (const auto byte : digest) {
    text += kDigits[byte >> 4];
    text += kDigits[byte & 15];
  }
  return text;
}

} // namespace

TEST_CASE("md5Hex matches the RFC 1321 test suite") {
  CHECK(md5Hex("") == "d41d8cd98f00b204e9800998ecf8427e");
  CHECK(md5Hex("a") == "0cc175b9c0f1b6a831c399e269772661");
  CHECK(md5Hex("abc") == "900150983cd24fb0d6963f7d28e17f72");
  CHECK(md5Hex("message digest") == "f96b697d7cb7938d525a2f31aaf161d0");
  CHECK(
      md5Hex("12345678901234567890123456789012345678901234567890123456789012345678901234567890") ==
      "57edf4a22be3c955ac49da2e2107b67a");
}

TEST_CASE("sha1 matches the FIPS 180 examples") {
  CHECK(hex(sha1("")) == "da39a3ee5e6b4b0d3255bfef95601890afd80709");
  CHECK(hex(sha1("abc")) == "a9993e364706816aba3e25717850c26c9cd0d89d");
  CHECK(
      hex(sha1("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq")) ==
      "84983e441c3bd26ebaae4aa1f95129e5e54670f1");
}

TEST_CASE("sha1 pads a message whose length leaves no room for its size") {
  const std::string block(56, 'a');
  const std::string twoBlocks(64, 'a');

  CHECK(hex(sha1(block)) == "c2db330f6083854c99d4b5bfb6e8f29f201be699");
  CHECK(hex(sha1(twoBlocks)) == "0098ba824b5c16427bd7a1122a5a442a25ec644d");
}

TEST_CASE("sha1 of a million a's matches FIPS 180") {
  const std::string million(1000000, 'a');

  CHECK(hex(sha1(million)) == "34aa973cd4c4daa4f61eeb2bdbad27316534016f");
}

} // namespace rntizen
