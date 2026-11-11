#include "intl/IcuData.h"

#include "logging/Log.h"

#include <unicode/putil.h>
#include <unicode/uloc.h>
#include <unicode/utypes.h>

#include <sys/stat.h>

#include <array>
#include <cstring>
#include <string_view>

namespace rntizen {
namespace {

std::string languageOf(const char *locale) {
  std::array<char, ULOC_LANG_CAPACITY> language{};
  UErrorCode status = U_ZERO_ERROR;
  uloc_getLanguage(locale, language.data(), static_cast<int32_t>(language.size()), &status);
  return U_SUCCESS(status) ? language.data() : "";
}

const char *shippedLocaleFor(std::string_view language) {
  const char *first = nullptr;
  for (int32_t index = 0; index < uloc_countAvailable(); index += 1) {
    const char *locale = uloc_getAvailable(index);
    if (languageOf(locale) == language) return nullptr;
    if (first == nullptr && std::strcmp(locale, "root") != 0) first = locale;
  }
  return first;
}

} // namespace

void useIcuData(const std::string &resourceDir) {
  const std::string file = resourceDir + "/" U_ICUDATA_NAME ".dat";
  if (struct stat info {}; stat(file.c_str(), &info) != 0) {
    log(LogLevel::Error, "No ICU data at " + file + ": Intl and locale-aware strings will fail");
    return;
  }
  u_setDataDirectory(resourceDir.c_str());
  const std::string device = uloc_getDefault();
  const char *fallback = shippedLocaleFor(languageOf(device.c_str()));
  if (fallback == nullptr) return;
  UErrorCode status = U_ZERO_ERROR;
  uloc_setDefault(fallback, &status);
  log(LogLevel::Info, "The app ships no ICU data for " + device + ": formatting in " + fallback);
}

} // namespace rntizen
