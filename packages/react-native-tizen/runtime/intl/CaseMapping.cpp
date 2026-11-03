#include "hermes/Platform/Intl/PlatformIntl.h"

#include <unicode/uloc.h>
#include <unicode/ustring.h>

#include <string>
#include <vector>

namespace hermes {
namespace platform_intl {
namespace {

using CaseFunction = int32_t (*)(UChar *, int32_t, const UChar *, int32_t, const char *, UErrorCode *);

// ECMA-402 TransformCase: the first requested locale, else the default; ICU
// reads only its language, which is what picks Turkish, Azeri or Lithuanian rules.
std::u16string transformCase(
    const std::vector<std::u16string> &locales,
    const std::u16string &text,
    CaseFunction convert) {
  std::string requested;
  if (!locales.empty()) {
    for (const char16_t unit : locales.front()) requested += static_cast<char>(unit);
  }
  const char *locale = requested.empty() ? uloc_getDefault() : requested.c_str();
  const auto *source = reinterpret_cast<const UChar *>(text.data());
  const auto length = static_cast<int32_t>(text.size());
  UErrorCode status = U_ZERO_ERROR;
  const int32_t needed = convert(nullptr, 0, source, length, locale, &status);
  if (status != U_BUFFER_OVERFLOW_ERROR && U_FAILURE(status)) return text;
  std::u16string result(static_cast<size_t>(needed), u'\0');
  status = U_ZERO_ERROR;
  convert(reinterpret_cast<UChar *>(result.data()), needed, source, length, locale, &status);
  return U_SUCCESS(status) ? result : text;
}

} // namespace

vm::CallResult<std::u16string> toLocaleLowerCase(
    vm::Runtime &,
    const std::vector<std::u16string> &locales,
    const std::u16string &str) {
  return transformCase(locales, str, u_strToLower);
}

vm::CallResult<std::u16string> toLocaleUpperCase(
    vm::Runtime &,
    const std::vector<std::u16string> &locales,
    const std::u16string &str) {
  return transformCase(locales, str, u_strToUpper);
}

} // namespace platform_intl
} // namespace hermes
