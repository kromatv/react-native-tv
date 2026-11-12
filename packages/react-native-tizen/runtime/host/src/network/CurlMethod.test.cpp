#include "CurlMethod.h"

#include <doctest/doctest.h>

#include <optional>
#include <string>

namespace rntizen {

TEST_CASE("a GET goes without the body JavaScript gave it") {
  const std::optional<std::string> body = "ignored";

  const auto sent = curlMethodFor("GET", body);

  CHECK_FALSE(sent.noBody);
  CHECK_FALSE(sent.customRequest.has_value());
  CHECK_FALSE(sent.body.has_value());
  CHECK_FALSE(curlMethodFor("get", body).body.has_value());
}

TEST_CASE("a HEAD asks for no body and sends none") {
  const auto sent = curlMethodFor("HEAD", std::string("ignored"));

  CHECK(sent.noBody);
  CHECK_FALSE(sent.customRequest.has_value());
  CHECK_FALSE(sent.body.has_value());
}

TEST_CASE("any other method is sent as it is named, with its body") {
  const std::optional<std::string> body = R"({"a":1})";

  const auto sent = curlMethodFor("PUT", body);

  CHECK(sent.customRequest == "PUT");
  REQUIRE(sent.body.has_value());
  CHECK(*sent.body == R"({"a":1})");
  CHECK(curlMethodFor("DELETE", body).body == R"({"a":1})");
}

TEST_CASE("a POST, PUT or PATCH without a body carries an empty one, other methods none") {
  CHECK(curlMethodFor("POST", std::nullopt).body == std::string_view());
  CHECK(curlMethodFor("patch", std::nullopt).body == std::string_view());
  CHECK(curlMethodFor("PUT", std::nullopt).body.has_value());

  CHECK_FALSE(curlMethodFor("DELETE", std::nullopt).body.has_value());
  CHECK(curlMethodFor("OPTIONS", std::nullopt).customRequest == "OPTIONS");
}

} // namespace rntizen
