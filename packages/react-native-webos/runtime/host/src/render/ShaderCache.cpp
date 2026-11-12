#include "ShaderCache.h"

#include <array>
#include <bit>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <span>
#include <system_error>
#include <utility>
#include <vector>

namespace rnwebos {

namespace {

namespace fs = std::filesystem;

constexpr std::array<char, 4> kMagic{'R', 'N', 'S', 'C'};
constexpr std::size_t kMaxKeyBytes = 4096;
constexpr std::size_t kMaxProgramBytes = 4 * 1024 * 1024;
constexpr std::size_t kMaxStoredBytes = 16 * 1024 * 1024;

struct Header {
  std::array<char, 4> magic;
  std::uint32_t keyBytes;
  std::uint32_t programBytes;
};

std::uint64_t fnv1a(const SkData &data) {
  std::uint64_t hash = 0xcbf29ce484222325ULL;
  for (const auto byte : std::span(data.bytes(), data.size())) {
    hash = (hash ^ byte) * 0x100000001b3ULL;
  }
  return hash;
}

std::size_t bytesIn(const std::string &directory) {
  std::error_code error;
  std::size_t total = 0;
  for (const auto &entry : fs::directory_iterator(directory, error)) {
    const auto size = entry.file_size(error);
    if (!error) total += size;
  }
  return total;
}

} // namespace

ShaderCache::ShaderCache(std::string directory) : directory_(std::move(directory)) {
  std::error_code error;
  fs::create_directories(directory_, error);
  storedBytes_ = bytesIn(directory_);
}

std::string ShaderCache::pathOf(const SkData &key) const {
  return std::format("{}/{:016x}", directory_, fnv1a(key));
}

sk_sp<SkData> ShaderCache::load(const SkData &key) {
  std::ifstream file(pathOf(key), std::ios::binary);
  std::array<char, sizeof(Header)> packed{};
  if (!file.read(packed.data(), packed.size())) return nullptr;
  const auto header = std::bit_cast<Header>(packed);
  if (header.magic != kMagic || header.keyBytes != key.size() || header.programBytes == 0 ||
      header.programBytes > kMaxProgramBytes) {
    return nullptr;
  }
  std::vector<char> stored(header.keyBytes);
  if (!file.read(stored.data(), static_cast<std::streamsize>(stored.size()))) return nullptr;
  if (std::memcmp(stored.data(), key.data(), key.size()) != 0) return nullptr;
  auto program = SkData::MakeUninitialized(header.programBytes);
  if (!file.read(static_cast<char *>(program->writable_data()), header.programBytes)) return nullptr;
  return file.peek() == std::ifstream::traits_type::eof() ? program : nullptr;
}

void ShaderCache::store(const SkData &key, const SkData &data, const SkString &) {
  const auto bytes = sizeof(Header) + key.size() + data.size();
  if (key.size() > kMaxKeyBytes || data.empty() || data.size() > kMaxProgramBytes) return;
  if (storedBytes_ + bytes > kMaxStoredBytes) return;
  const auto path = pathOf(key);
  const auto partial = path + ".partial";
  const auto header = std::bit_cast<std::array<char, sizeof(Header)>>(Header{
      .magic = kMagic,
      .keyBytes = static_cast<std::uint32_t>(key.size()),
      .programBytes = static_cast<std::uint32_t>(data.size()),
  });
  {
    std::ofstream file(partial, std::ios::binary | std::ios::trunc);
    file.write(header.data(), header.size());
    file.write(static_cast<const char *>(key.data()), static_cast<std::streamsize>(key.size()));
    file.write(static_cast<const char *>(data.data()), static_cast<std::streamsize>(data.size()));
    if (!file) return;
  }
  // A program half written when the app was killed must never be loaded.
  std::error_code error;
  fs::rename(partial, path, error);
  if (!error) storedBytes_ += bytes;
}

} // namespace rnwebos
