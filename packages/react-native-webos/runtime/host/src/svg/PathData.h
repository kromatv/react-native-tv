#pragma once

#include "SvgPath.h"

#include <string_view>

namespace rntizen::svg {

/// Parses SVG path data (`d`): every command, absolute and relative, with
/// implicit repeats, smooth curves and packed arc flags. As SVG asks, data
/// after the first error is dropped and the path up to it is kept.
Path parsePathData(std::string_view data);

} // namespace rntizen::svg
