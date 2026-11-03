#pragma once

#include <jsi/jsi.h>

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace rntizen {

/// Headers as React Native's Android networking passes them: `[name, value]` pairs, in order.
using HeaderPairs = std::vector<std::pair<std::string, std::string>>;

HeaderPairs readHeaderPairs(facebook::jsi::Runtime &runtime, const facebook::jsi::Array &headers);

/// The value of the first header named `name`, compared without case; empty when there is none.
std::string_view headerValue(const HeaderPairs &headers, std::string_view name);

/// Replaces every header named `name`, compared without case, with one `name: value`.
void setHeader(HeaderPairs &headers, std::string_view name, std::string_view value);

} // namespace rntizen
