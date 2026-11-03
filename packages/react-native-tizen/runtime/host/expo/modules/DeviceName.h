#pragma once

#include <string>

namespace rntizen::expo {

/// The name the viewer gave the TV in its settings, or `fallback` when the
/// platform does not say.
std::string tvDeviceName(const std::string &fallback);

} // namespace rntizen::expo
