#include "TizenVersion.h"

#include <charconv>

namespace rntizen {

namespace {

std::optional<int> leadingNumber(std::string_view &text) {
  int value = 0;
  const auto *end = text.data() + text.size();
  const auto [next, error] = std::from_chars(text.data(), end, value);
  if (error != std::errc{} || next == text.data()) return std::nullopt;
  text.remove_prefix(static_cast<std::size_t>(next - text.data()));
  return value;
}

} // namespace

std::optional<TizenVersion> parseTizenVersion(std::string_view text) {
  const auto major = leadingNumber(text);
  if (!major.has_value() || text.empty() || text.front() != '.') return std::nullopt;
  text.remove_prefix(1);
  const auto minor = leadingNumber(text);
  if (!minor.has_value()) return std::nullopt;
  return TizenVersion{.major = *major, .minor = *minor};
}

double platformVersionNumber(std::string_view text) {
  const auto version = parseTizenVersion(text);
  return version ? version->major + version->minor / 10.0 : 0;
}

} // namespace rntizen
