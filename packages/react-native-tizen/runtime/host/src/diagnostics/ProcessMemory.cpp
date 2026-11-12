#include "ProcessMemory.h"

#include <charconv>
#include <fstream>
#include <sstream>

namespace rntizen {

namespace {

constexpr double kKilobytesPerMb = 1024;
constexpr std::size_t kMaxProcFileBytes = 64 * 1024;

std::optional<std::string> readSmall(const std::string &path) {
  std::ifstream file(path);
  if (!file) return std::nullopt;
  std::string text(kMaxProcFileBytes, '\0');
  file.read(text.data(), static_cast<std::streamsize>(text.size()));
  text.resize(static_cast<std::size_t>(file.gcount()));
  return text;
}

} // namespace

std::optional<double> kilobytesOf(std::string_view text, std::string_view key) {
  for (std::size_t start = 0; start < text.size();) {
    const std::size_t end = std::min(text.find('\n', start), text.size());
    const std::string_view line = text.substr(start, end - start);
    start = end + 1;
    if (line.size() <= key.size() || !line.starts_with(key) || line[key.size()] != ':') continue;
    std::string_view value = line.substr(key.size() + 1);
    const std::size_t digits = value.find_first_not_of(' ');
    if (digits == std::string_view::npos) return std::nullopt;
    value = value.substr(digits);
    double kilobytes = 0;
    const auto [rest, error] = std::from_chars(value.data(), value.data() + value.size(), kilobytes);
    if (error != std::errc{} || rest == value.data()) return std::nullopt;
    return kilobytes;
  }
  return std::nullopt;
}

ProcessMemory readProcessMemory(const std::string &procSelf) {
  if (const auto rollup = readSmall(procSelf + "/smaps_rollup")) {
    if (const auto pss = kilobytesOf(*rollup, "Pss"); pss.has_value()) {
      return {.pssMb = *pss / kKilobytesPerMb,
              .swapMb = kilobytesOf(*rollup, "SwapPss").value_or(0) / kKilobytesPerMb,
              .source = "smaps_rollup"};
    }
  }
  if (const auto status = readSmall(procSelf + "/status")) {
    if (const auto rss = kilobytesOf(*status, "VmRSS"); rss.has_value()) {
      return {.pssMb = *rss / kKilobytesPerMb,
              .swapMb = kilobytesOf(*status, "VmSwap").value_or(0) / kKilobytesPerMb,
              .source = "status"};
    }
  }
  return {};
}

} // namespace rntizen
