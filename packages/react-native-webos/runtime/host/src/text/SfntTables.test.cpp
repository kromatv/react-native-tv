#include "SfntTables.h"

#include <doctest/doctest.h>

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace rntizen {

namespace {

struct TableSpec {
  const char *tag;
  std::string data;
};

void putU16(std::string &bytes, std::size_t at, int value) {
  bytes[at] = static_cast<char>((value >> 8) & 0xff);
  bytes[at + 1] = static_cast<char>(value & 0xff);
}

void putU32(std::string &bytes, std::size_t at, std::uint32_t value) {
  putU16(bytes, at, static_cast<int>(value >> 16));
  putU16(bytes, at + 2, static_cast<int>(value & 0xffff));
}

int readU16(const std::string &bytes, std::size_t at) {
  return (static_cast<unsigned char>(bytes[at]) << 8) | static_cast<unsigned char>(bytes[at + 1]);
}

std::string font(const std::vector<TableSpec> &tables) {
  std::string bytes(12 + 16 * tables.size(), '\0');
  putU32(bytes, 0, 0x00010000);
  putU16(bytes, 4, static_cast<int>(tables.size()));
  for (std::size_t i = 0; i < tables.size(); ++i) {
    const auto record = 12 + 16 * i;
    bytes.replace(record, 4, tables[i].tag, 4);
    putU32(bytes, record + 8, static_cast<std::uint32_t>(bytes.size()));
    putU32(bytes, record + 12, static_cast<std::uint32_t>(tables[i].data.size()));
    bytes += tables[i].data;
  }
  return bytes;
}

std::string head(int unitsPerEm) {
  std::string table(54, '\0');
  putU16(table, 18, unitsPerEm);
  return table;
}

std::string hhea(int metrics) {
  std::string table(36, '\0');
  putU16(table, 34, metrics);
  return table;
}

std::string hmtx(const std::vector<int> &advances) {
  std::string table(4 * advances.size(), '\0');
  for (std::size_t i = 0; i < advances.size(); ++i) putU16(table, 4 * i, advances[i]);
  return table;
}

std::string os2(int weight, bool italic) {
  std::string table(78, '\0');
  putU16(table, 4, weight);
  putU16(table, 62, italic ? 0x1 : 0x40);
  return table;
}

std::size_t hmtxOffset(const std::string &bytes) {
  for (std::size_t record = 12; record + 16 <= bytes.size(); record += 16) {
    if (bytes.compare(record, 4, "hmtx") == 0) {
      return (static_cast<std::size_t>(readU16(bytes, record + 8)) << 16) | static_cast<std::size_t>(readU16(bytes, record + 10));
    }
  }
  return 0;
}

std::vector<int> advancesOf(const std::string &bytes, std::size_t count) {
  const auto offset = hmtxOffset(bytes);
  std::vector<int> advances;
  for (std::size_t i = 0; i < count; ++i) advances.push_back(readU16(bytes, offset + 4 * i));
  return advances;
}

} // namespace

TEST_CASE("sfntStyle reads the OS/2 weight class and italic bit") {
  const auto style = sfntStyle(font({{"OS/2", os2(600, true)}}));

  REQUIRE(style.has_value());
  CHECK(style->weight == 600);
  CHECK(style->italic);
  CHECK_FALSE(sfntStyle(font({{"OS/2", os2(400, false)}}))->italic);
}

TEST_CASE("sfntStyle gives nothing for a collection, a short OS/2 or none") {
  auto collection = font({{"OS/2", os2(400, false)}});
  collection.replace(0, 4, "ttcf");

  CHECK_FALSE(sfntStyle(collection).has_value());
  CHECK_FALSE(sfntStyle(font({{"OS/2", std::string(40, '\0')}})).has_value());
  CHECK_FALSE(sfntStyle(font({{"head", head(1000)}})).has_value());
  CHECK_FALSE(sfntStyle("").has_value());
}

TEST_CASE("sfntUnitsPerEm reads head and refuses zero") {
  CHECK(sfntUnitsPerEm(font({{"head", head(2048)}})) == 2048);
  CHECK_FALSE(sfntUnitsPerEm(font({{"head", head(0)}})).has_value());
  CHECK_FALSE(sfntUnitsPerEm(font({{"OS/2", os2(400, false)}})).has_value());
}

TEST_CASE("trackSfntAdvances adds the spacing to every advance width") {
  auto bytes = font({{"head", head(1000)}, {"hhea", hhea(3)}, {"hmtx", hmtx({500, 0, 1200})}});
  const auto size = bytes.size();

  REQUIRE(trackSfntAdvances(bytes, 50));

  CHECK(advancesOf(bytes, 3) == std::vector{550, 50, 1250});
  CHECK(bytes.size() == size);
}

TEST_CASE("trackSfntAdvances tightens down to zero and no further") {
  auto bytes = font({{"hhea", hhea(2)}, {"hmtx", hmtx({500, 30})}});

  REQUIRE(trackSfntAdvances(bytes, -100));

  CHECK(advancesOf(bytes, 2) == std::vector{400, 0});
}

TEST_CASE("trackSfntAdvances stops at the largest advance a font can hold") {
  auto bytes = font({{"hhea", hhea(1)}, {"hmtx", hmtx({65500})}});

  REQUIRE(trackSfntAdvances(bytes, 100));

  CHECK(advancesOf(bytes, 1) == std::vector{0xffff});
}

TEST_CASE("trackSfntAdvances refuses a font whose metrics do not fit its hmtx") {
  auto bytes = font({{"hhea", hhea(5)}, {"hmtx", hmtx({500, 500})}});
  const auto before = bytes;

  CHECK_FALSE(trackSfntAdvances(bytes, 10));
  CHECK(bytes == before);
  auto missing = font({{"hmtx", hmtx({500})}});
  CHECK_FALSE(trackSfntAdvances(missing, 10));
}

} // namespace rntizen
