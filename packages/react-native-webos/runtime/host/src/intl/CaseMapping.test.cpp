#include <doctest/doctest.h>

#include <hermes/hermes.h>
#include <jsi/jsi.h>

#include <memory>
#include <string>

namespace {

std::string evaluate(const char *source) {
  auto runtime = facebook::hermes::makeHermesRuntime();
  const auto result = runtime->evaluateJavaScript(std::make_shared<facebook::jsi::StringBuffer>(source), "case.js");
  return result.getString(*runtime).utf8(*runtime);
}

} // namespace

TEST_CASE("toLocaleLowerCase and toLocaleUpperCase follow the requested locale's rules") {
  CHECK(evaluate("'TITLE'.toLocaleLowerCase('tr')") == "t\xC4\xB1tle");
  CHECK(evaluate("'title'.toLocaleUpperCase('tr')") == "T\xC4\xB0TLE");
}

TEST_CASE("toLocaleLowerCase without a locale uses the default one") {
  CHECK(evaluate("'\xC3\x89T\xC3\x89'.toLocaleLowerCase()") == "\xC3\xA9t\xC3\xA9");
}

TEST_CASE("toLocaleUpperCase of an empty string is empty") {
  CHECK(evaluate("''.toLocaleUpperCase('tr')").empty());
}
