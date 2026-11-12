#include "MultipartBody.h"

#include <doctest/doctest.h>

#include <regex>
#include <set>

namespace rntizen {

TEST_CASE("a multipart boundary is the fixed prefix and 32 random hex digits, within RFC 2046's 70 characters") {
  const std::regex layout("RNTizenFormBoundary[0-9a-f]{32}");
  std::set<std::string> seen;
  for (int index = 0; index < 32; ++index) {
    const auto boundary = multipartBoundary();
    CHECK(std::regex_match(boundary, layout));
    CHECK(boundary.size() <= 70);
    seen.insert(boundary);
  }
  CHECK(seen.size() == 32);
}

TEST_CASE("encodeMultipart lays out each part's headers, a blank line and its bytes, then the closing boundary") {
  const std::vector<MultipartPart> parts{
      {.headers = {{"content-disposition", "form-data; name=\"a\""}}, .body = "line one\r\nline two"},
      {.headers = {}, .body = std::string("\0binary\xff", 8)},
  };

  CHECK(encodeMultipart(parts, "b") ==
        "--b\r\ncontent-disposition: form-data; name=\"a\"\r\n\r\nline one\r\nline two\r\n"
        "--b\r\n\r\n" + std::string("\0binary\xff", 8) + "\r\n"
        "--b--\r\n");
  CHECK(encodeMultipart({}, "b") == "--b--\r\n");
}

} // namespace rntizen
