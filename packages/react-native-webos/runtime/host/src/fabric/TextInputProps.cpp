#include "TextInputProps.h"

#include <react/renderer/core/propsConversions.h>

namespace rntizen {

using facebook::react::convertRawProp;

TextInputProps::TextInputProps(
    const facebook::react::PropsParserContext &context,
    const TextInputProps &sourceProps,
    const facebook::react::RawProps &rawProps)
    : BaseTextInputProps(context, sourceProps, rawProps),
      secureTextEntry(convertRawProp(context, rawProps, "secureTextEntry", sourceProps.secureTextEntry, false)),
      autoCorrect(convertRawProp(context, rawProps, "autoCorrect", sourceProps.autoCorrect, false)),
      selectTextOnFocus(convertRawProp(context, rawProps, "selectTextOnFocus", sourceProps.selectTextOnFocus, false)),
      caretHidden(convertRawProp(context, rawProps, "caretHidden", sourceProps.caretHidden, false)),
      showSoftInputOnFocus(convertRawProp(
          context,
          rawProps,
          "showSoftInputOnFocus",
          sourceProps.showSoftInputOnFocus,
          std::optional<bool>{})),
      keyboardType(convertRawProp(context, rawProps, "keyboardType", sourceProps.keyboardType, std::string{})),
      returnKeyType(convertRawProp(context, rawProps, "returnKeyType", sourceProps.returnKeyType, std::string{})) {}

} // namespace rntizen
