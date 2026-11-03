#pragma once

#include <string>

namespace rntizen {

/// Points ICU at the `icudt<major>l.dat` the app ships in `resourceDir`, the
/// data Hermes' Intl and Unicode functions read. Call it before the runtime
/// starts.
void useIcuData(const std::string &resourceDir);

} // namespace rntizen
