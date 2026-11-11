#pragma once

#include <stdexcept>

namespace rntizen {

/// A request or connection that failed: its message is what JavaScript sees.
class NetworkError final : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

} // namespace rntizen
