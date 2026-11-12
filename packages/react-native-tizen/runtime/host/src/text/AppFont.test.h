#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

namespace rntizen::testing {

inline void putU16(std::string &bytes, std::size_t at, std::uint32_t value) {
  bytes[at] = static_cast<char>((value >> 8) & 0xff);
  bytes[at + 1] = static_cast<char>(value & 0xff);
}

inline void putU32(std::string &bytes, std::size_t at, std::uint32_t value) {
  putU16(bytes, at, value >> 16);
  putU16(bytes, at + 2, value & 0xffff);
}

inline std::string appFont(std::uint32_t weight, std::uint32_t unitsPerEm) {
  std::string os2(78, '\0');
  putU16(os2, 4, weight);
  std::string head(54, '\0');
  putU16(head, 18, unitsPerEm);
  std::string hhea(36, '\0');
  putU16(hhea, 34, 1);
  std::string hmtx(4, '\0');
  putU16(hmtx, 0, 500);
  const std::vector<std::pair<std::string, std::string>> tables{
      {"OS/2", os2}, {"head", head}, {"hhea", hhea}, {"hmtx", hmtx}};
  std::string bytes(12 + 16 * tables.size(), '\0');
  putU32(bytes, 0, 0x00010000);
  putU16(bytes, 4, static_cast<std::uint32_t>(tables.size()));
  for (std::size_t index = 0; index < tables.size(); ++index) {
    const auto record = 12 + 16 * index;
    bytes.replace(record, 4, tables[index].first);
    putU32(bytes, record + 8, static_cast<std::uint32_t>(bytes.size()));
    putU32(bytes, record + 12, static_cast<std::uint32_t>(tables[index].second.size()));
    bytes += tables[index].second;
  }
  return bytes;
}

inline std::filesystem::path writeFile(const std::filesystem::path &file, const std::string &bytes) {
  std::ofstream(file, std::ios::binary) << bytes;
  return file;
}

} // namespace rntizen::testing
