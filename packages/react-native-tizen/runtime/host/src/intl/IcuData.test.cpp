#include "IcuData.h"

#include <doctest/doctest.h>

#include <unicode/putil.h>
#include <unicode/uclean.h>
#include <unicode/uloc.h>
#include <unicode/utypes.h>

#include <algorithm>
#include <cstdint>
#include <ranges>
#include <string>
#include <string_view>

namespace rntizen {

namespace {

constexpr const char *kIcuDataDirectory = RN_TIZEN_TEST_ICU_DATA_DIR;

class FreshIcu {
 public:
  FreshIcu() {
    u_cleanup();
  }
  ~FreshIcu() {
    u_cleanup();
  }
  FreshIcu(const FreshIcu &) = delete;
  FreshIcu &operator=(const FreshIcu &) = delete;
};

void setDeviceLocale(const char *locale) {
  UErrorCode status = U_ZERO_ERROR;
  uloc_setDefault(locale, &status);
  REQUIRE(U_SUCCESS(status));
}

bool isShipped(std::string_view locale) {
  const auto isLocale = [locale](int32_t index) { return locale == uloc_getAvailable(index); };
  return std::ranges::any_of(std::views::iota(int32_t{0}, uloc_countAvailable()), isLocale);
}

} // namespace

TEST_CASE_FIXTURE(FreshIcu, "useIcuData leaves ICU alone when the package has no data file") {
  const std::string locale = uloc_getDefault();
  const std::string dataDirectory = u_getDataDirectory();

  useIcuData("/nonexistent/rn-tizen-icu");

  CHECK(locale == uloc_getDefault());
  CHECK(dataDirectory == u_getDataDirectory());
}

TEST_CASE_FIXTURE(FreshIcu, "useIcuData points ICU at the package's data file and keeps a device locale it ships") {
  setDeviceLocale("fr_FR");

  useIcuData(kIcuDataDirectory);

  CHECK(std::string(u_getDataDirectory()) == kIcuDataDirectory);
  CHECK(std::string(uloc_getDefault()) == "fr_FR");
}

TEST_CASE_FIXTURE(FreshIcu, "useIcuData picks a shipped locale when the package has none for the device's language") {
  setDeviceLocale("xx_XX");

  useIcuData(kIcuDataDirectory);

  const std::string fallback = uloc_getDefault();
  CHECK(fallback != "xx_XX");
  CHECK(isShipped(fallback));
}

} // namespace rntizen
