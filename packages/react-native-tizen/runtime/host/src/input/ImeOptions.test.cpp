#include "ImeSession.h"

#include <doctest/doctest.h>

#include <string>
#include <tuple>
#include <vector>

namespace rntizen {

namespace {

ImeOptions keyboardOptions(const std::string &keyboardType) {
  return imeOptionsFor(keyboardType, "", "", false, false, false);
}

} // namespace

TEST_CASE("a field with no keyboard props gets the normal layout, Done, sentence capitals and no prediction") {
  const auto options = imeOptionsFor("", "", "", false, false, false);

  CHECK(options.layout == ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL);
  CHECK(options.layoutVariation == 0);
  CHECK(options.returnKey == ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_DONE);
  CHECK(options.autocapital == ECORE_IMF_AUTOCAPITAL_TYPE_SENTENCE);
  CHECK_FALSE(options.prediction);
  CHECK_FALSE(options.secure);
  CHECK_FALSE(options.multiline);
  CHECK(options.showPanel);
}

TEST_CASE("each keyboardType opens its panel layout, the numeric ones with Android's sign and decimal point") {
  for (const auto &[keyboardType, layout, variation] :
       std::vector<std::tuple<std::string, Ecore_IMF_Input_Panel_Layout, int>>{
           {"default", ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL, 0},
           {"ascii-capable", ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL, 0},
           {"email-address", ECORE_IMF_INPUT_PANEL_LAYOUT_EMAIL, 0},
           {"url", ECORE_IMF_INPUT_PANEL_LAYOUT_URL, 0},
           {"phone-pad", ECORE_IMF_INPUT_PANEL_LAYOUT_PHONENUMBER, 0},
           {"visible-password", ECORE_IMF_INPUT_PANEL_LAYOUT_PASSWORD, 0},
           {"numeric",
            ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBERONLY,
            ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBERONLY_VARIATION_SIGNED_AND_DECIMAL},
           {"number-pad",
            ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBERONLY,
            ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBERONLY_VARIATION_NORMAL},
           {"decimal-pad",
            ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBERONLY,
            ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBERONLY_VARIATION_DECIMAL},
       }) {
    CAPTURE(keyboardType);
    const auto options = keyboardOptions(keyboardType);

    CHECK(options.layout == layout);
    CHECK(options.layoutVariation == variation);
  }
}

TEST_CASE("each returnKeyType names the panel's return key, and any other one is Done") {
  for (const auto &[returnKeyType, returnKey] :
       std::vector<std::pair<std::string, Ecore_IMF_Input_Panel_Return_Key_Type>>{
           {"go", ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_GO},
           {"next", ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_NEXT},
           {"search", ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_SEARCH},
           {"send", ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_SEND},
           {"join", ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_JOIN},
           {"done", ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_DONE},
           {"previous", ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_DONE},
       }) {
    CAPTURE(returnKeyType);
    CHECK(imeOptionsFor("", returnKeyType, "", false, false, false).returnKey == returnKey);
  }
}

TEST_CASE("each autoCapitalize sets the panel's capitals, and any other one capitalizes sentences") {
  for (const auto &[autoCapitalize, autocapital] : std::vector<std::pair<std::string, Ecore_IMF_Autocapital_Type>>{
           {"none", ECORE_IMF_AUTOCAPITAL_TYPE_NONE},
           {"words", ECORE_IMF_AUTOCAPITAL_TYPE_WORD},
           {"characters", ECORE_IMF_AUTOCAPITAL_TYPE_ALLCHARACTER},
           {"sentences", ECORE_IMF_AUTOCAPITAL_TYPE_SENTENCE},
       }) {
    CAPTURE(autoCapitalize);
    CHECK(imeOptionsFor("", "", autoCapitalize, false, false, false).autocapital == autocapital);
  }
}

TEST_CASE("autoCorrect turns prediction on, and multiline asks for a multiline panel") {
  const auto options = imeOptionsFor("", "", "", true, false, true);

  CHECK(options.prediction);
  CHECK(options.multiline);
  CHECK_FALSE(options.secure);
}

TEST_CASE("a secure field gets the password layout with no capitals and no prediction") {
  const auto options = imeOptionsFor("", "", "words", true, true, false);

  CHECK(options.layout == ECORE_IMF_INPUT_PANEL_LAYOUT_PASSWORD);
  CHECK(options.autocapital == ECORE_IMF_AUTOCAPITAL_TYPE_NONE);
  CHECK_FALSE(options.prediction);
  CHECK(options.secure);
}

TEST_CASE("a secure numeric field keeps its number layout, as Android's numeric password does") {
  const auto options = imeOptionsFor("number-pad", "", "", false, true, false);

  CHECK(options.layout == ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBERONLY);
  CHECK(options.layoutVariation == ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBERONLY_VARIATION_NORMAL);
  CHECK(options.secure);
}

} // namespace rntizen
