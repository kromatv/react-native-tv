#pragma once

#include "include/core/SkPath.h"

#include <string_view>

namespace rnwebos::svg {

/// Parses SVG path data (`d`): every command, absolute and relative, with
/// implicit repeats, smooth curves and packed arc flags. As SVG asks, data
/// after the first error is dropped and the path up to it is kept.
SkPath parsePathData(std::string_view data);

} // namespace rnwebos::svg
