#include "WebOSVersion.h"

#include <charconv>
#include <optional>

namespace rnwebos {

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

double platformVersionNumber(std::string_view text) {
  const auto major = leadingNumber(text);
  if (!major.has_value() || text.empty() || text.front() != '.') return 0;
  text.remove_prefix(1);
  const auto minor = leadingNumber(text);
  if (!minor.has_value() || *minor > 99) return 0;
  return *major + *minor / 100.0;
}

} // namespace rnwebos
