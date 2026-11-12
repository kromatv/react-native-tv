#pragma once

#include <string_view>

namespace rnwebos {

/// The TV's `sdkVersion` as the number `Platform.Version` is: the major plus a
/// hundredth per minor, since minors reach two digits (4.10), so 10.02 for
/// `10.2.0` and 1.05 for `1.05.00`. 0 unless `text` starts with `<digits>.<digits>`.
double platformVersionNumber(std::string_view text);

} // namespace rnwebos
