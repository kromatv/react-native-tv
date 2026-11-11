#include "ImeSession.h"

#include <array>
#include <format>

namespace rnwebos {

namespace {

constexpr std::array<std::string_view, 3> kNumberKeyboards{"numeric", "number-pad", "decimal-pad"};
constexpr std::array<std::string_view, 2> kKeyboardsSdlLacks{"url", "phone-pad"};
constexpr std::array<std::string_view, 3> kReturnKeysShown{"", "default", "done"};

template <std::size_t Count>
bool isOneOf(std::string_view value, const std::array<std::string_view, Count> &values) {
  for (const auto candidate : values) {
    if (candidate == value) return true;
  }
  return false;
}

SDL_TextInputType typeFor(std::string_view keyboardType, bool secure) {
  if (isOneOf(keyboardType, kNumberKeyboards)) {
    return secure ? SDL_TEXTINPUT_TYPE_NUMBER_PASSWORD_HIDDEN : SDL_TEXTINPUT_TYPE_NUMBER;
  }
  if (secure) return SDL_TEXTINPUT_TYPE_TEXT_PASSWORD_HIDDEN;
  if (keyboardType == "email-address") return SDL_TEXTINPUT_TYPE_TEXT_EMAIL;
  if (keyboardType == "visible-password") return SDL_TEXTINPUT_TYPE_TEXT_PASSWORD_VISIBLE;
  return SDL_TEXTINPUT_TYPE_TEXT;
}

SDL_Capitalization capitalizationFor(std::string_view autoCapitalize) {
  if (autoCapitalize == "none") return SDL_CAPITALIZE_NONE;
  if (autoCapitalize == "words") return SDL_CAPITALIZE_WORDS;
  if (autoCapitalize == "characters") return SDL_CAPITALIZE_LETTERS;
  return SDL_CAPITALIZE_SENTENCES;
}

} // namespace

ImeOptions imeOptionsFor(const KeyboardProps &props) {
  ImeOptions options{};
  options.type = typeFor(props.keyboardType, props.secure);
  options.capitalization = props.secure ? SDL_CAPITALIZE_NONE : capitalizationFor(props.autoCapitalize);
  options.autocorrect = props.autoCorrect && !props.secure;
  options.multiline = props.multiline;
  if (isOneOf(props.keyboardType, kKeyboardsSdlLacks)) {
    options.unsupported.push_back(
        std::format("keyboardType \"{}\" has no SDL text input type: the keyboard opens as for text", props.keyboardType));
  }
  if (!props.multiline && !isOneOf(props.returnKeyType, kReturnKeysShown)) {
    options.unsupported.push_back(
        std::format("returnKeyType \"{}\" cannot be set through SDL: the keyboard's action key says Done", props.returnKeyType));
  }
  if (!props.showOnFocus) {
    options.unsupported.emplace_back(
        "showSoftInputOnFocus={false} cannot be honoured: on webOS starting text input raises the keyboard");
  }
  return options;
}

} // namespace rnwebos
