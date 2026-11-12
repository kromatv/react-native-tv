#include "MultipartBody.h"

#include <cstdint>
#include <format>
#include <iterator>
#include <random>

namespace rntizen {

std::string multipartBoundary() {
  std::random_device device;
  const std::uint64_t high = (std::uint64_t{device()} << 32) | device();
  const std::uint64_t low = (std::uint64_t{device()} << 32) | device();
  return std::format("RNTizenFormBoundary{:016x}{:016x}", high, low);
}

std::string encodeMultipart(const std::vector<MultipartPart> &parts, std::string_view boundary) {
  std::string body;
  auto out = std::back_inserter(body);
  for (const auto &part : parts) {
    std::format_to(out, "--{}\r\n", boundary);
    for (const auto &[name, value] : part.headers) std::format_to(out, "{}: {}\r\n", name, value);
    std::format_to(out, "\r\n{}\r\n", part.body);
  }
  std::format_to(out, "--{}--\r\n", boundary);
  return body;
}

} // namespace rntizen
