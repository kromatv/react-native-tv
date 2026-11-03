#include "FileIo.h"

#include "Uuid.h"

#include <fstream>
#include <iterator>
#include <stdexcept>
#include <system_error>

namespace rntizen::expo {

namespace {

int hexValue(char digit) {
  if (digit >= '0' && digit <= '9') return digit - '0';
  if (digit >= 'a' && digit <= 'f') return digit - 'a' + 10;
  if (digit >= 'A' && digit <= 'F') return digit - 'A' + 10;
  return -1;
}

std::string percentDecoded(std::string_view text) {
  std::string decoded;
  decoded.reserve(text.size());
  for (std::size_t index = 0; index < text.size(); ++index) {
    if (text[index] == '%' && index + 2 < text.size()) {
      const int high = hexValue(text[index + 1]);
      const int low = hexValue(text[index + 2]);
      if (high >= 0 && low >= 0) {
        decoded += static_cast<char>(high * 16 + low);
        index += 2;
        continue;
      }
    }
    decoded += text[index];
  }
  return decoded;
}

} // namespace

std::optional<std::string> readFile(const std::filesystem::path &path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) return std::nullopt;
  std::string data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  if (file.bad()) return std::nullopt;
  return data;
}

void writeFileAtomically(const std::filesystem::path &path, std::string_view data) {
  const auto temporary = path.parent_path() / ("." + path.filename().string() + "." + uuidV4() + ".tmp");
  {
    std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
    file.write(data.data(), static_cast<std::streamsize>(data.size()));
    file.close();
    if (!file) {
      std::error_code ignored;
      std::filesystem::remove(temporary, ignored);
      throw std::runtime_error("Cannot write " + path.string());
    }
  }
  std::error_code error;
  std::filesystem::rename(temporary, path, error);
  if (error) {
    std::filesystem::remove(temporary, error);
    throw std::runtime_error("Cannot write " + path.string());
  }
}

std::optional<std::string> localPathOf(const std::string &uri) {
  constexpr std::string_view scheme = "file://";
  if (uri.rfind(scheme, 0) == 0) {
    auto rest = std::string_view(uri).substr(scheme.size());
    const auto end = rest.find_first_of("?#");
    return percentDecoded(rest.substr(0, end));
  }
  if (!uri.empty() && uri.front() == '/') return uri;
  return std::nullopt;
}

} // namespace rntizen::expo
