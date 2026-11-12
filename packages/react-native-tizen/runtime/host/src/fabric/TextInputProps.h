#pragma once

#include <react/renderer/components/textinput/BaseTextInputProps.h>

#include <optional>
#include <string>

namespace rntizen {

/// The props of `AndroidTextInput` the Tizen view reads: React Native's
/// cross-platform text input props plus the keyboard ones Android parses in
/// `AndroidTextInputProps`, whose header needs Android's serializable props.
/// `showSoftInputOnFocus` stays unset when JavaScript leaves it out, so the
/// keyboard opens on focus by default, as an EditText's does.
class TextInputProps final : public facebook::react::BaseTextInputProps {
 public:
  TextInputProps() = default;
  TextInputProps(
      const facebook::react::PropsParserContext &context,
      const TextInputProps &sourceProps,
      const facebook::react::RawProps &rawProps);

  bool secureTextEntry{false};
  bool autoCorrect{false};
  bool selectTextOnFocus{false};
  bool caretHidden{false};
  std::optional<bool> showSoftInputOnFocus{};
  std::string keyboardType{};
  std::string returnKeyType{};
};

} // namespace rntizen
