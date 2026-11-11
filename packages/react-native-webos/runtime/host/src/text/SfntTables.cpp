#include "SfntTables.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace rntizen {

namespace {

struct Table {
  std::size_t offset;
  std::size_t length;
};

std::uint32_t byteAt(const std::string &bytes, std::size_t at) {
  return std::to_integer<std::uint32_t>(static_cast<std::byte>(bytes[at]));
}

std::uint32_t readU32(const std::string &bytes, std::size_t at) {
  return (byteAt(bytes, at) << 24) | (byteAt(bytes, at + 1) << 16) | (byteAt(bytes, at + 2) << 8) | byteAt(bytes, at + 3);
}

int readU16(const std::string &bytes, std::size_t at) {
  return static_cast<int>((byteAt(bytes, at) << 8) | byteAt(bytes, at + 1));
}

void writeU16(std::string &bytes, std::size_t at, int value) {
  bytes[at] = static_cast<char>((value >> 8) & 0xff);
  bytes[at + 1] = static_cast<char>(value & 0xff);
}

// OpenType "Table Directory": sfntVersion, numTables, then 16-byte records.
std::optional<Table> findTable(const std::string &bytes, const char *tag) {
  if (bytes.size() < 12 || bytes.compare(0, 4, "ttcf") == 0) return std::nullopt;
  const int count = readU16(bytes, 4);
  for (int index = 0; index < count; ++index) {
    const std::size_t record = 12 + 16 * static_cast<std::size_t>(index);
    if (record + 16 > bytes.size()) return std::nullopt;
    if (bytes.compare(record, 4, tag) != 0) continue;
    const Table table{readU32(bytes, record + 8), readU32(bytes, record + 12)};
    if (table.offset + table.length > bytes.size()) return std::nullopt;
    return table;
  }
  return std::nullopt;
}

} // namespace

std::optional<SfntStyle> sfntStyle(const std::string &bytes) {
  const auto os2 = findTable(bytes, "OS/2");
  if (!os2 || os2->length < 64) return std::nullopt;
  return SfntStyle{
      .weight = readU16(bytes, os2->offset + 4),
      .italic = (readU16(bytes, os2->offset + 62) & 0x1) != 0,
  };
}

std::optional<int> sfntUnitsPerEm(const std::string &bytes) {
  const auto head = findTable(bytes, "head");
  if (!head || head->length < 20) return std::nullopt;
  const int units = readU16(bytes, head->offset + 18);
  return units > 0 ? std::optional(units) : std::nullopt;
}

bool trackSfntAdvances(std::string &bytes, int units) {
  const auto hhea = findTable(bytes, "hhea");
  const auto hmtx = findTable(bytes, "hmtx");
  if (!hhea || !hmtx || hhea->length < 36) return false;
  const auto metrics = static_cast<std::size_t>(readU16(bytes, hhea->offset + 34));
  if (metrics * 4 > hmtx->length) return false;
  for (std::size_t index = 0; index < metrics; ++index) {
    const auto at = hmtx->offset + index * 4;
    writeU16(bytes, at, std::clamp(readU16(bytes, at) + units, 0, 0xffff));
  }
  return true;
}

} // namespace rntizen
