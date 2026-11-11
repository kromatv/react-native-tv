#include "ImeSession.h"

#include <doctest/doctest.h>

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace rnwebos {

namespace {

bool mentions(const ImeOptions &options, std::string_view words) {
  for (const auto &sentence : options.unsupported) {
    if (sentence.find(words) != std::string::npos) return true;
  }
  return false;
}

} // namespace

TEST_CASE("a field with no keyboard props is text, capitalized by sentence, with no autocorrect") {
  const auto options = imeOptionsFor({});

  CHECK(options.type == SDL_TEXTINPUT_TYPE_TEXT);
  CHECK(options.capitalization == SDL_CAPITALIZE_SENTENCES);
  CHECK_FALSE(options.autocorrect);
  CHECK_FALSE(options.multiline);
  CHECK(options.unsupported.empty());
}

TEST_CASE("each keyboardType SDL can say gets its text input type, and the number ones are all a number") {
  for (const auto &[keyboardType, type] : std::vector<std::pair<std::string_view, SDL_TextInputType>>{
           {"default", SDL_TEXTINPUT_TYPE_TEXT},
           {"ascii-capable", SDL_TEXTINPUT_TYPE_TEXT},
           {"email-address", SDL_TEXTINPUT_TYPE_TEXT_EMAIL},
           {"visible-password", SDL_TEXTINPUT_TYPE_TEXT_PASSWORD_VISIBLE},
           {"numeric", SDL_TEXTINPUT_TYPE_NUMBER},
           {"number-pad", SDL_TEXTINPUT_TYPE_NUMBER},
           {"decimal-pad", SDL_TEXTINPUT_TYPE_NUMBER},
       }) {
    CAPTURE(keyboardType);
    const auto options = imeOptionsFor({.keyboardType = keyboardType});

    CHECK(options.type == type);
    CHECK(options.unsupported.empty());
  }
}

TEST_CASE("url and phone-pad open the text keyboard and say that SDL has no type for them") {
  for (const std::string_view keyboardType : {"url", "phone-pad"}) {
    CAPTURE(keyboardType);
    const auto options = imeOptionsFor({.keyboardType = keyboardType});

    CHECK(options.type == SDL_TEXTINPUT_TYPE_TEXT);
    REQUIRE(options.unsupported.size() == 1);
    CHECK(mentions(options, keyboardType));
  }
}

TEST_CASE("a returnKeyType other than done is reported on a single line, where the key says Done") {
  for (const std::string_view returnKeyType : {"", "default", "done"}) {
    CAPTURE(returnKeyType);
    CHECK(imeOptionsFor({.returnKeyType = returnKeyType}).unsupported.empty());
  }
  for (const std::string_view returnKeyType : {"go", "next", "search", "send", "previous", "none"}) {
    CAPTURE(returnKeyType);
    const auto options = imeOptionsFor({.returnKeyType = returnKeyType});

    REQUIRE(options.unsupported.size() == 1);
    CHECK(mentions(options, returnKeyType));
  }
}

TEST_CASE("a multiline field reports no returnKeyType: its key is Return whatever the prop says") {
  const auto options = imeOptionsFor({.returnKeyType = "search", .multiline = true});

  CHECK(options.multiline);
  CHECK(options.unsupported.empty());
}

TEST_CASE("each autoCapitalize sets SDL's capitalization, and any other one capitalizes sentences") {
  for (const auto &[autoCapitalize, capitalization] : std::vector<std::pair<std::string_view, SDL_Capitalization>>{
           {"none", SDL_CAPITALIZE_NONE},
           {"words", SDL_CAPITALIZE_WORDS},
           {"characters", SDL_CAPITALIZE_LETTERS},
           {"sentences", SDL_CAPITALIZE_SENTENCES},
           {"", SDL_CAPITALIZE_SENTENCES},
       }) {
    CAPTURE(autoCapitalize);
    CHECK(imeOptionsFor({.autoCapitalize = autoCapitalize}).capitalization == capitalization);
  }
}

TEST_CASE("autoCorrect turns SDL's autocorrect on") {
  CHECK(imeOptionsFor({.autoCorrect = true}).autocorrect);
}

TEST_CASE("a secure field is a hidden password with no capitals and no autocorrect, whatever its text keyboard") {
  for (const std::string_view keyboardType : {"", "email-address", "visible-password"}) {
    CAPTURE(keyboardType);
    const auto options =
        imeOptionsFor({.keyboardType = keyboardType, .autoCapitalize = "words", .autoCorrect = true, .secure = true});

    CHECK(options.type == SDL_TEXTINPUT_TYPE_TEXT_PASSWORD_HIDDEN);
    CHECK(options.capitalization == SDL_CAPITALIZE_NONE);
    CHECK_FALSE(options.autocorrect);
  }
}

TEST_CASE("a secure number field is a hidden PIN, as Android's numeric password is") {
  CHECK(imeOptionsFor({.keyboardType = "number-pad", .secure = true}).type == SDL_TEXTINPUT_TYPE_NUMBER_PASSWORD_HIDDEN);
}

TEST_CASE("showSoftInputOnFocus false is reported: starting text input raises the keyboard on webOS") {
  const auto options = imeOptionsFor({.showOnFocus = false});

  REQUIRE(options.unsupported.size() == 1);
  CHECK(mentions(options, "showSoftInputOnFocus"));
}

} // namespace rnwebos
