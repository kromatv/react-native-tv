#include "ImeSession.h"

namespace rntizen {

namespace {

void applyKeyboardType(ImeOptions &options, const std::string &keyboardType) {
  if (keyboardType == "email-address") {
    options.layout = ECORE_IMF_INPUT_PANEL_LAYOUT_EMAIL;
  } else if (keyboardType == "url") {
    options.layout = ECORE_IMF_INPUT_PANEL_LAYOUT_URL;
  } else if (keyboardType == "phone-pad") {
    options.layout = ECORE_IMF_INPUT_PANEL_LAYOUT_PHONENUMBER;
  } else if (keyboardType == "visible-password") {
    options.layout = ECORE_IMF_INPUT_PANEL_LAYOUT_PASSWORD;
  } else if (keyboardType == "numeric") {
    options.layout = ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBERONLY;
    options.layoutVariation = ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBERONLY_VARIATION_SIGNED_AND_DECIMAL;
  } else if (keyboardType == "number-pad") {
    options.layout = ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBERONLY;
    options.layoutVariation = ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBERONLY_VARIATION_NORMAL;
  } else if (keyboardType == "decimal-pad") {
    options.layout = ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBERONLY;
    options.layoutVariation = ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBERONLY_VARIATION_DECIMAL;
  }
}

Ecore_IMF_Input_Panel_Return_Key_Type returnKeyFor(const std::string &returnKeyType) {
  if (returnKeyType == "go") return ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_GO;
  if (returnKeyType == "next") return ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_NEXT;
  if (returnKeyType == "search") return ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_SEARCH;
  if (returnKeyType == "send") return ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_SEND;
  if (returnKeyType == "join") return ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_JOIN;
  return ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_DONE;
}

Ecore_IMF_Autocapital_Type autocapitalFor(const std::string &autoCapitalize) {
  if (autoCapitalize == "none") return ECORE_IMF_AUTOCAPITAL_TYPE_NONE;
  if (autoCapitalize == "words") return ECORE_IMF_AUTOCAPITAL_TYPE_WORD;
  if (autoCapitalize == "characters") return ECORE_IMF_AUTOCAPITAL_TYPE_ALLCHARACTER;
  return ECORE_IMF_AUTOCAPITAL_TYPE_SENTENCE;
}

} // namespace

ImeOptions imeOptionsFor(
    const std::string &keyboardType,
    const std::string &returnKeyType,
    const std::string &autoCapitalize,
    bool autoCorrect,
    bool secure,
    bool multiline) {
  ImeOptions options{};
  applyKeyboardType(options, keyboardType);
  if (secure && options.layout == ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL) options.layout = ECORE_IMF_INPUT_PANEL_LAYOUT_PASSWORD;
  options.returnKey = returnKeyFor(returnKeyType);
  options.autocapital = secure ? ECORE_IMF_AUTOCAPITAL_TYPE_NONE : autocapitalFor(autoCapitalize);
  options.prediction = autoCorrect && !secure;
  options.secure = secure;
  options.multiline = multiline;
  return options;
}

} // namespace rntizen
