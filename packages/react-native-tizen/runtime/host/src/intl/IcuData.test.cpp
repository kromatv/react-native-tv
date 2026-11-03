#include "IcuData.h"

#include <doctest/doctest.h>

#include <unicode/putil.h>
#include <unicode/uloc.h>
#include <unicode/utypes.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace rntizen {

namespace fs = std::filesystem;

TEST_CASE("useIcuData leaves ICU alone when the package has no data file") {
  const std::string locale = uloc_getDefault();

  useIcuData("/nonexistent/rn-tizen-icu");

  CHECK(locale == uloc_getDefault());
}

TEST_CASE("useIcuData points ICU at the package's data file and keeps a locale it ships") {
  const auto directory = fs::temp_directory_path() / "rn-tizen-icu-data-test";
  fs::create_directories(directory);
  std::ofstream(directory / (U_ICUDATA_NAME ".dat")) << "";
  const std::string dataDirectory = u_getDataDirectory();
  const std::string locale = uloc_getDefault();

  useIcuData(directory.string());

  CHECK(std::string(u_getDataDirectory()) == directory.string());
  UErrorCode status = U_ZERO_ERROR;
  uloc_setDefault(locale.c_str(), &status);
  u_setDataDirectory(dataDirectory.c_str());
  fs::remove_all(directory);
}

} // namespace rntizen
