#include "HeaderPairs.h"
#include "MultipartBody.h"

#include "../TurboModuleTest.test.h"

#include <doctest/doctest.h>

#include <regex>

namespace rntizen {

TEST_CASE("header pairs come from JavaScript's [name, value] arrays, values turned into strings") {
  auto runtime = testing::makeRuntime();
  auto &rt = *runtime;
  const auto pairs = readHeaderPairs(rt, testing::evaluate(rt, "[['Content-Type', 'text/plain'], ['X-Count', 2]]").asObject(rt).asArray(rt));
  CHECK(pairs == HeaderPairs{{"Content-Type", "text/plain"}, {"X-Count", "2"}});
}

TEST_CASE("headerValue finds the first header of a name in any case, and setHeader replaces every one") {
  HeaderPairs headers{{"Accept", "a"}, {"content-type", "text/plain"}, {"CONTENT-TYPE", "text/html"}};
  CHECK(headerValue(headers, "Content-Type") == "text/plain");
  CHECK(headerValue(headers, "X-Missing").empty());
  CHECK(headerValue(headers, "Accep").empty());

  setHeader(headers, "Content-Type", "multipart/form-data; boundary=b");
  CHECK(headers == HeaderPairs{{"Accept", "a"}, {"Content-Type", "multipart/form-data; boundary=b"}});
}

TEST_CASE("a multipart boundary is random hex after a fixed prefix") {
  const std::regex layout("RNTizenFormBoundary[0-9a-f]{32}");
  const auto first = multipartBoundary();
  CHECK(std::regex_match(first, layout));
  CHECK(first != multipartBoundary());
}

TEST_CASE("encodeMultipart lays out each part after its boundary line, then the closing boundary") {
  const std::vector<MultipartPart> parts{
      {.headers = {{"content-disposition", "form-data; name=\"a\""}}, .body = "1"},
      {.headers = {{"content-disposition", "form-data; name=\"f\"; filename=\"f.txt\""}, {"Content-Type", "text/plain"}},
       .body = "two\r\nlines"},
  };
  CHECK(encodeMultipart(parts, "B") ==
        "--B\r\ncontent-disposition: form-data; name=\"a\"\r\n\r\n1\r\n"
        "--B\r\ncontent-disposition: form-data; name=\"f\"; filename=\"f.txt\"\r\nContent-Type: text/plain\r\n\r\ntwo\r\nlines\r\n"
        "--B--\r\n");
  CHECK(encodeMultipart({}, "B") == "--B--\r\n");
}

} // namespace rntizen
