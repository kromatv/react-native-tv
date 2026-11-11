#pragma once

#include <optional>
#include <string_view>

namespace rnwebos {

/// A webOS platform version as `system_info` reports it, such as `6.5` or `10.0.0`.
struct WebOSVersion {
  int major{0};
  int minor{0};

  auto operator<=>(const WebOSVersion &) const = default;
};

/// The major and minor numbers of `text`; nullopt unless it starts with `<digits>.<digits>`.
std::optional<WebOSVersion> parseWebOSVersion(std::string_view text);

/// The version as `Platform.Version` gives it: 6.5 for webOS 6.5, 0 when unknown.
/// webOS minor versions are one digit.
double platformVersionNumber(std::string_view text);

} // namespace rnwebos
