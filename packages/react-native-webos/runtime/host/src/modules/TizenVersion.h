#pragma once

#include <optional>
#include <string_view>

namespace rntizen {

/// A Tizen platform version as `system_info` reports it, such as `6.5` or `10.0.0`.
struct TizenVersion {
  int major{0};
  int minor{0};

  auto operator<=>(const TizenVersion &) const = default;
};

/// The major and minor numbers of `text`; nullopt unless it starts with `<digits>.<digits>`.
std::optional<TizenVersion> parseTizenVersion(std::string_view text);

/// The version as `Platform.Version` gives it: 6.5 for Tizen 6.5, 0 when unknown.
/// Tizen minor versions are one digit.
double platformVersionNumber(std::string_view text);

} // namespace rntizen
