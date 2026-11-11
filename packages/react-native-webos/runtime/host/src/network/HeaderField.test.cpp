#include "HeaderField.h"

#include <doctest/doctest.h>

#include <string>

namespace rntizen {

TEST_CASE("isHeaderName accepts tokens and nothing else") {
  CHECK(isHeaderName("Content-Type"));
  CHECK(isHeaderName("x-custom_header.v2!#$%&'*+^`|~"));

  CHECK_FALSE(isHeaderName(""));
  CHECK_FALSE(isHeaderName("Bad Name"));
  CHECK_FALSE(isHeaderName("X-Evil\r\nHost"));
  CHECK_FALSE(isHeaderName("Name:"));
  CHECK_FALSE(isHeaderName("caf\xc3\xa9"));
}

TEST_CASE("isHeaderValue refuses NUL, CR and LF") {
  CHECK(isHeaderValue(""));
  CHECK(isHeaderValue("text/html; charset=utf-8"));
  CHECK(isHeaderValue("tab\tand caf\xc3\xa9"));

  CHECK_FALSE(isHeaderValue("a\r\nX-Injected: 1"));
  CHECK_FALSE(isHeaderValue("a\nb"));
  CHECK_FALSE(isHeaderValue("a\rb"));
  CHECK_FALSE(isHeaderValue(std::string("a\0b", 3)));
}

TEST_CASE("invalidHeader names the first header that cannot be sent") {
  CHECK_FALSE(invalidHeader({{"Accept", "*/*"}, {"X-Id", "7"}}).has_value());

  CHECK(invalidHeader({{"Accept", "*/*"}, {"X-Id", "7\r\nCookie: stolen"}, {"Bad Name", "x"}}) == "X-Id");
  CHECK(invalidHeader({{"Bad Name", "x"}}) == "Bad Name");
}

} // namespace rntizen
