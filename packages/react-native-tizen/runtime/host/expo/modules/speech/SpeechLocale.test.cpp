#include "SpeechLocale.h"

#include <doctest/doctest.h>

#include <string>
#include <vector>

namespace rntizen::expo::speech {

TEST_CASE("a BCP 47 request finds the engine's language") {
  const std::vector<std::string> supported{"en_US", "en_GB", "fr_FR", "de_DE"};

  CHECK(sttLanguageFor("en-GB", supported, "fr_FR") == "en_GB");
  CHECK(sttLanguageFor("EN_us", supported, "fr_FR") == "en_US");
  CHECK(sttLanguageFor("fr", supported, "fr_FR") == "fr_FR");
  CHECK(sttLanguageFor("fr-CA", supported, "fr_FR") == "fr_FR");
  CHECK(sttLanguageFor("de", supported, "fr_FR") == "de_DE");
  CHECK(sttLanguageFor("", supported, "fr_FR") == "fr_FR");
  CHECK(sttLanguageFor("fr", supported, "") == "fr_FR");
  CHECK_FALSE(sttLanguageFor("ja-JP", supported, "fr_FR").has_value());
  CHECK(toBcp47("fr_FR") == "fr-FR");
}

} // namespace rntizen::expo::speech
