#include <unicode/putil.h>

namespace {

// SkParagraph breaks lines with ICU's rules, which are in the data file an app
// ships and not in the test binary: without it the text engine cannot start.
const bool icuDataReady = [] {
  u_setDataDirectory(RN_WEBOS_TEST_ICU_DATA_DIR);
  return true;
}();

} // namespace
